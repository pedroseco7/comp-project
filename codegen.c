#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h> 
#include "semantics.h"

int temporary;   
int label_counter = 0; 
int declare_str_id = 0; 
int use_str_id = 0;     
int block_terminated = 0;
char *current_method_name = NULL;
ExprType current_method_ret_type = TypeVoid;
char *current_string_array_param = NULL; // Guarda o nome do array de argumentos!

struct node *global_root_ast = NULL;

extern ExprType get_expr_type(enum category cat);
int codegen_expression(struct node *expr);
void codegen_method(struct node *method_node);
void codegen_field(struct node *field_node);

struct local_var_info {
    char *name;
    int reg_ptr;
    ExprType type;
};
struct local_var_info local_vars[1000];
int num_local_vars = 0;

struct local_var_info global_vars[1000];
int num_global_vars = 0;

int get_local_var(char *name) {
    // Procura as variáveis mais recentes primeiro (respeitando o scope!)
    for (int i = num_local_vars - 1; i >= 0; i--) {
        if (strcmp(local_vars[i].name, name) == 0) return local_vars[i].reg_ptr;
    }
    return -1; 
}

const char* get_llvm_type(ExprType type) {
    switch(type) {
        case TypeInt: return "i32";
        case TypeDouble: return "double";
        case TypeBoolean: return "i1";
        case TypeVoid: return "void";
        default: return "i32"; 
    }
}

ExprType get_actual_type(struct node *expr) {
    if (!expr) return TypeVoid;
    switch (expr->category) {
        case Natural: case Length: return TypeInt;
        case Decimal: return TypeDouble;
        case ParseArgs: {
            struct node *id_node = expr->children->node;
            if (id_node && strstr(id_node->token, "Double")) return TypeDouble;
            return TypeInt;
        }
        case BoolLit: case Eq: case Ne: case Lt: case Le: case Gt: case Ge: case And: case Or: case Xor: case Not: return TypeBoolean;
        case Identifier: {
            if (current_string_array_param && strcmp(expr->token, current_string_array_param) == 0) return TypeVoid;
            for (int i = num_local_vars - 1; i >= 0; i--) {
                if (strcmp(local_vars[i].name, expr->token) == 0) return local_vars[i].type;
            }
            for (int i = 0; i < num_global_vars; i++) {
                if (strcmp(global_vars[i].name, expr->token) == 0) return global_vars[i].type;
            }
            return expr->type;
        }
        case Plus: case Minus: return get_actual_type(expr->children->node);
        case Add: case Sub: case Mul: case Div: case Mod:
            if (get_actual_type(expr->children->node) == TypeDouble || get_actual_type(expr->children->next->node) == TypeDouble) return TypeDouble;
            return TypeInt;
        // A CURA DO DIVISIONS (n2 = n1 = 0): Analisa sempre a variável alvo, não o valor bruto!
        case Assign: return get_actual_type(expr->children->node); 
        case Call: return expr->type;
        default: return expr->type;
    }
}

int promote_to_double(int reg, ExprType current_type) {
    if (current_type == TypeInt) {
        int tmp = temporary++;
        printf("  %%%d = sitofp i32 %%%d to double\n", tmp, reg);
        return tmp;
    }
    return reg;
}

void get_mangled_name(struct node *method_node, char *buffer) {
    struct node *header = method_node->children->node;
    struct node *id_node = header->children->next->node;
    
    if (strcmp(id_node->token, "main") == 0) {
        struct node *params = header->children->next->next->node;
        if (params->children && params->children->node->children->node->category == StringArray) {
            strcpy(buffer, "main");
            return;
        }
    }
    
    sprintf(buffer, "m_%s", id_node->token); 
    struct node *params = header->children->next->next->node;
    struct node_list *p = params->children;
    while (p) {
        struct node *type_node = p->node->children->node;
        if (type_node->category == StringArray) strcat(buffer, "_strarr");
        else {
            ExprType pt = get_expr_type(type_node->category);
            if (pt == TypeInt) strcat(buffer, "_i");
            else if (pt == TypeDouble) strcat(buffer, "_d");
            else if (pt == TypeBoolean) strcat(buffer, "_b");
            else strcat(buffer, "_unknown"); 
        }
        p = p->next;
    }
}

int get_llvm_string_length(const char *token) {
    int len = strlen(token);
    int start = (token[0] == '"') ? 1 : 0;
    int end = (token[len-1] == '"') ? len - 1 : len;
    int real_len = 0;
    for (int i = start; i < end; i++) {
        if (token[i] == '\\' && i + 1 < end) { i++; }
        real_len++;
    }
    return real_len + 1; 
}

void print_llvm_string_literal(const char *token) {
    int len = strlen(token);
    int start = (token[0] == '"') ? 1 : 0;
    int end = (token[len-1] == '"') ? len - 1 : len;
    for (int i = start; i < end; i++) {
        unsigned char c = token[i];
        if (c == '\\' && i + 1 < end) {
            char next = token[i+1];
            if (next == 'n') { printf("\\0A"); i++; }
            else if (next == 't') { printf("\\09"); i++; }
            else if (next == 'r') { printf("\\0D"); i++; } 
            else if (next == 'f') { printf("\\0C"); i++; } 
            else if (next == 'b') { printf("\\08"); i++; } 
            else if (next == '\\') { printf("\\5C"); i++; }
            else if (next == '"') { printf("\\22"); i++; }
            else if (next == '\'') { printf("\\27"); i++; }
            else { printf("\\%02X", next); i++; }
        } 
        else if (c == '\n') { printf("\\0A"); }
        else if (c == '\r') { printf("\\0D"); }
        else if (c == '\t') { printf("\\09"); }
        else if (c == '\f') { printf("\\0C"); }
        else if (c == '\b') { printf("\\08"); }
        else if (c == '"')  { printf("\\22"); }
        else {
            printf("%c", c);
        }
    }
    printf("\\00");
}

void codegen_field(struct node *field_node) {
    struct node *type_node = field_node->children->node;
    struct node_list *id_list = field_node->children->next; 
    const char *ll_type = get_llvm_type(get_expr_type(type_node->category));
    
    while (id_list != NULL) {
        struct node *id_node = id_list->node;
        global_vars[num_global_vars].name = id_node->token;
        global_vars[num_global_vars].type = get_expr_type(type_node->category);
        num_global_vars++;
        
        if (get_expr_type(type_node->category) == TypeDouble) {
            printf("@%s = global %s 0.0\n", id_node->token, ll_type);
        } else {
            printf("@%s = global %s 0\n", id_node->token, ll_type);
        }
        id_list = id_list->next;
    }
}

void declare_global_strings(struct node *current) {
    if (!current) return;
    if (current->category == Print && current->children->node->category == StrLit) {
        struct node *str_node = current->children->node;
        int real_len = get_llvm_string_length(str_node->token);
        printf("@.str.%d = private unnamed_addr constant [%d x i8] c\"", declare_str_id, real_len);
        print_llvm_string_literal(str_node->token);
        printf("\"\n");
        declare_str_id++;
    }
    struct node_list *child = current->children;
    while (child != NULL) {
        declare_global_strings(child->node);
        child = child->next;
    }
}

void emit_label(int label) {
    printf("L%d:\n", label);
    block_terminated = 0; 
}

void emit_br(int label) {
    if (!block_terminated) {
        printf("  br label %%L%d\n", label);
        block_terminated = 1;
    }
}

void codegen_statement(struct node *stmt) {
    if (!stmt || block_terminated) return;
    
    // A CURA DO COMPLEX (Escopo Nativo do Java): A variável só existe DEPOIS de declarada!
    if (stmt->category == VarDecl) {
        struct node *v_type_node = stmt->children->node;
        struct node_list *id_list = stmt->children->next; 
        const char *ll_type = get_llvm_type(get_expr_type(v_type_node->category));
        
        while (id_list != NULL) {
            struct node *v_id_node = id_list->node;
            int ptr_reg = temporary++;
            
            printf("  %%%d = alloca %s\n", ptr_reg, ll_type);
            if (get_expr_type(v_type_node->category) == TypeDouble) {
                printf("  store double 0.0, double* %%%d\n", ptr_reg);
            } else if (get_expr_type(v_type_node->category) == TypeBoolean) {
                printf("  store i1 0, i1* %%%d\n", ptr_reg);
            } else {
                printf("  store i32 0, i32* %%%d\n", ptr_reg);
            }

            local_vars[num_local_vars].name = v_id_node->token;
            local_vars[num_local_vars].reg_ptr = ptr_reg;
            local_vars[num_local_vars].type = get_expr_type(v_type_node->category);
            num_local_vars++;
            
            id_list = id_list->next;
        }
        return;
    }
    else if (stmt->category == Print) {
        struct node *expr = stmt->children->node;
        
        if (expr->category != StrLit) {
            int reg = codegen_expression(expr);
            ExprType actual_type = get_actual_type(expr);
            
            if (actual_type == TypeInt) {
                int ptr = temporary++;
                int call_reg = temporary++; 
                printf("  %%%d = getelementptr [3 x i8], [3 x i8]* @.str.int, i32 0, i32 0\n", ptr);
                printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d, i32 %%%d)\n", call_reg, ptr, reg);
            } 
            else if (actual_type == TypeDouble) {
                int ptr = temporary++;
                int call_reg = temporary++; 
                printf("  %%%d = getelementptr [6 x i8], [6 x i8]* @.str.double, i32 0, i32 0\n", ptr);
                printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d, double %%%d)\n", call_reg, ptr, reg);
            } 
            else if (actual_type == TypeBoolean) {
                int ptr_true = temporary++;
                printf("  %%%d = getelementptr [5 x i8], [5 x i8]* @.str.true, i32 0, i32 0\n", ptr_true);
                int ptr_false = temporary++;
                printf("  %%%d = getelementptr [6 x i8], [6 x i8]* @.str.false, i32 0, i32 0\n", ptr_false);
                int str_ptr = temporary++;
                printf("  %%%d = select i1 %%%d, i8* %%%d, i8* %%%d\n", str_ptr, reg, ptr_true, ptr_false);
                int call_reg = temporary++;
                printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)\n", call_reg, str_ptr);
            }
        }
        else { 
            int str_ptr = temporary++;
            int str_id = use_str_id++; 
            int real_len = get_llvm_string_length(expr->token);
            printf("  %%%d = getelementptr [%d x i8], [%d x i8]* @.str.%d, i32 0, i32 0\n", 
                   str_ptr, real_len, real_len, str_id);
            int fmt_ptr = temporary++;
            printf("  %%%d = getelementptr [3 x i8], [3 x i8]* @.str.str, i32 0, i32 0\n", fmt_ptr);
            int call_reg = temporary++;
            printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d, i8* %%%d)\n", call_reg, fmt_ptr, str_ptr);
        }
    }
    else if (stmt->category == If) {
        int cond = codegen_expression(stmt->children->node);
        int then_block = label_counter++;
        int else_block = label_counter++;
        int end_block = label_counter++;
        if (!block_terminated) {
            printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond, then_block, else_block);
            block_terminated = 1;
        }
        emit_label(then_block);
        int saved_then = num_local_vars; // Salva o escopo local!
        codegen_statement(stmt->children->next->node);
        num_local_vars = saved_then;     // Restaura o escopo!
        emit_br(end_block);
        
        emit_label(else_block);
        if (stmt->children->next->next) { 
            int saved_else = num_local_vars;
            codegen_statement(stmt->children->next->next->node);
            num_local_vars = saved_else;
        }
        emit_br(end_block);
        emit_label(end_block);
    }
    else if (stmt->category == While) {
        int cond_block = label_counter++;
        int body_block = label_counter++;
        int end_block = label_counter++;
        emit_br(cond_block);
        
        emit_label(cond_block);
        int cond = codegen_expression(stmt->children->node);
        if (!block_terminated) {
            printf("  br i1 %%%d, label %%L%d, label %%L%d\n", cond, body_block, end_block);
            block_terminated = 1;
        }
        emit_label(body_block);
        int saved_body = num_local_vars; // Salva o escopo do loop!
        codegen_statement(stmt->children->next->node);
        num_local_vars = saved_body;     // Restaura o escopo!
        emit_br(cond_block);
        emit_label(end_block);
    }
    else if (stmt->category == Return) {
        if (!block_terminated) {
            if (stmt->children && stmt->children->node) {
                int ret_val = codegen_expression(stmt->children->node);
                ExprType val_actual = get_actual_type(stmt->children->node);
                if (current_method_ret_type == TypeDouble && val_actual == TypeInt) {
                    ret_val = promote_to_double(ret_val, TypeInt);
                }
                printf("  ret %s %%%d\n", get_llvm_type(current_method_ret_type), ret_val);
            } else {
                if (current_method_name && strcmp(current_method_name, "main") == 0) {
                    printf("  ret i32 0\n"); 
                } else {
                    printf("  ret void\n");
                }
            }
            block_terminated = 1;
        }
    }
    else if (stmt->category == Block) {
        int saved_block = num_local_vars; // Salva o escopo do bloco base!
        struct node_list *child = stmt->children;
        while (child != NULL) {
            codegen_statement(child->node);
            child = child->next;
        }
        num_local_vars = saved_block;     // Restaura o escopo!
    }
    else {
        codegen_expression(stmt);
    }
}

int codegen_expression(struct node *expr) {
    if (!expr) return -1;
    switch (expr->category) {
        case Natural: {
            int tmp = temporary++;
            char clean[256]; int j=0;
            for(int i=0; expr->token[i]; i++) if(expr->token[i] != '_') clean[j++] = expr->token[i];
            clean[j] = '\0';
            printf("  %%%d = add i32 0, %s\n", tmp, clean);
            return tmp;
        }
        case Decimal: {
            int tmp = temporary++;
            char clean[256]; int j=0;
            for(int i=0; expr->token[i]; i++) if(expr->token[i] != '_') clean[j++] = expr->token[i];
            clean[j] = '\0';
            double val = atof(clean); 
            printf("  %%%d = fadd double 0.0, %.16e\n", tmp, val); 
            return tmp;
        }
        case BoolLit: {
            int tmp = temporary++;
            printf("  %%%d = add i1 0, %d\n", tmp, strcmp(expr->token, "true") == 0 ? 1 : 0);
            return tmp;
        }
        case Identifier: {
            // A CURA DO RETURN_TYPES: Interceta arrays passados como argumento
            if (current_string_array_param && strcmp(expr->token, current_string_array_param) == 0) return -2;
            
            int ptr = get_local_var(expr->token);
            ExprType actual_type = get_actual_type(expr);
            const char *ll_type = get_llvm_type(actual_type);
            int tmp = temporary++; 
            if (ptr != -1) {
                printf("  %%%d = load %s, %s* %%%d\n", tmp, ll_type, ll_type, ptr);
            } else { 
                printf("  %%%d = load %s, %s* @%s\n", tmp, ll_type, ll_type, expr->token);
            }
            return tmp;
        }
        case Assign: {
            struct node *id_node = expr->children->node;
            struct node *val_node = expr->children->next->node;
            
            int val_reg = codegen_expression(val_node);
            ExprType val_actual = get_actual_type(val_node);
            ExprType id_actual = get_actual_type(id_node);
            
            if (id_actual == TypeDouble && val_actual == TypeInt) {
                val_reg = promote_to_double(val_reg, TypeInt);
            }
            
            int ptr = get_local_var(id_node->token);
            const char *ll_type = get_llvm_type(id_actual);
            
            if (ptr != -1) {
                printf("  store %s %%%d, %s* %%%d\n", ll_type, val_reg, ll_type, ptr);
            } else { 
                printf("  store %s %%%d, %s* @%s\n", ll_type, val_reg, ll_type, id_node->token);
            }
            return val_reg; 
        }
        case Add: case Sub: case Mul: case Div: case Mod: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            ExprType t1 = get_actual_type(expr->children->node);
            ExprType t2 = get_actual_type(expr->children->next->node);
            
            if (t1 == TypeDouble || t2 == TypeDouble) {
                e1 = promote_to_double(e1, t1);
                e2 = promote_to_double(e2, t2);
                int tmp = temporary++;
                if(expr->category == Add) printf("  %%%d = fadd double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Sub) printf("  %%%d = fsub double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Mul) printf("  %%%d = fmul double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Div) printf("  %%%d = fdiv double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Mod) printf("  %%%d = frem double %%%d, %%%d\n", tmp, e1, e2);
                return tmp;
            } else {
                int tmp = temporary++;
                if(expr->category == Add) printf("  %%%d = add i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Sub) printf("  %%%d = sub i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Mul) printf("  %%%d = mul i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Div) printf("  %%%d = sdiv i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Mod) printf("  %%%d = srem i32 %%%d, %%%d\n", tmp, e1, e2);
                return tmp;
            }
        }
        case Plus: return codegen_expression(expr->children->node);
        case Minus: {
            int e1 = codegen_expression(expr->children->node);
            int tmp = temporary++;
            if (get_actual_type(expr->children->node) == TypeDouble) {
                printf("  %%%d = fmul double %%%d, -1.0\n", tmp, e1);
            } else {
                printf("  %%%d = sub i32 0, %%%d\n", tmp, e1);
            }
            return tmp;
        }
        case Not: {
            int e1 = codegen_expression(expr->children->node);
            int tmp = temporary++;
            printf("  %%%d = xor i1 %%%d, 1\n", tmp, e1);
            return tmp;
        }
        case Eq: case Ne: case Lt: case Le: case Gt: case Ge: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            ExprType t1 = get_actual_type(expr->children->node);
            ExprType t2 = get_actual_type(expr->children->next->node);
            
            int is_double = (t1 == TypeDouble || t2 == TypeDouble);
            int is_bool = (t1 == TypeBoolean || t2 == TypeBoolean);
            
            if (is_double) {
                e1 = promote_to_double(e1, t1);
                e2 = promote_to_double(e2, t2);
                int tmp = temporary++;
                if(expr->category == Eq) printf("  %%%d = fcmp oeq double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Ne) printf("  %%%d = fcmp one double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Lt) printf("  %%%d = fcmp olt double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Le) printf("  %%%d = fcmp ole double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Gt) printf("  %%%d = fcmp ogt double %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Ge) printf("  %%%d = fcmp oge double %%%d, %%%d\n", tmp, e1, e2);
                return tmp;
            } else if (is_bool) {
                // A CURA DO RANDOMTEST (Boleanos usam nativo i1 no LLVM)
                int tmp = temporary++;
                if(expr->category == Eq) printf("  %%%d = icmp eq i1 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Ne) printf("  %%%d = icmp ne i1 %%%d, %%%d\n", tmp, e1, e2);
                return tmp;
            } else {
                int tmp = temporary++;
                if(expr->category == Eq) printf("  %%%d = icmp eq i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Ne) printf("  %%%d = icmp ne i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Lt) printf("  %%%d = icmp slt i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Le) printf("  %%%d = icmp sle i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Gt) printf("  %%%d = icmp sgt i32 %%%d, %%%d\n", tmp, e1, e2);
                if(expr->category == Ge) printf("  %%%d = icmp sge i32 %%%d, %%%d\n", tmp, e1, e2);
                return tmp;
            }
        }
        case And: {
            int res_ptr = temporary++;
            printf("  %%%d = alloca i1\n", res_ptr);
            int e1 = codegen_expression(expr->children->node);
            printf("  store i1 %%%d, i1* %%%d\n", e1, res_ptr);
            
            int eval_right = label_counter++;
            int end_block = label_counter++;
            
            if (!block_terminated) {
                printf("  br i1 %%%d, label %%L%d, label %%L%d\n", e1, eval_right, end_block);
                block_terminated = 1;
            }
            
            emit_label(eval_right);
            int e2 = codegen_expression(expr->children->next->node);
            if (!block_terminated) {
                printf("  store i1 %%%d, i1* %%%d\n", e2, res_ptr);
                printf("  br label %%L%d\n", end_block);
                block_terminated = 1;
            }
            
            emit_label(end_block);
            int tmp = temporary++;
            printf("  %%%d = load i1, i1* %%%d\n", tmp, res_ptr);
            return tmp;
        }
        case Or: {
            int res_ptr = temporary++;
            printf("  %%%d = alloca i1\n", res_ptr);
            int e1 = codegen_expression(expr->children->node);
            printf("  store i1 %%%d, i1* %%%d\n", e1, res_ptr);
            
            int eval_right = label_counter++;
            int end_block = label_counter++;
            
            if (!block_terminated) {
                printf("  br i1 %%%d, label %%L%d, label %%L%d\n", e1, end_block, eval_right);
                block_terminated = 1;
            }
            
            emit_label(eval_right);
            int e2 = codegen_expression(expr->children->next->node);
            if (!block_terminated) {
                printf("  store i1 %%%d, i1* %%%d\n", e2, res_ptr);
                printf("  br label %%L%d\n", end_block);
                block_terminated = 1;
            }
            
            emit_label(end_block);
            int tmp = temporary++;
            printf("  %%%d = load i1, i1* %%%d\n", tmp, res_ptr);
            return tmp;
        }
        case Lshift: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            printf("  %%%d = shl i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Rshift: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            printf("  %%%d = ashr i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Call: {
            struct node *id_node = expr->children->node;
            char *func_name = id_node->token;
            struct node_list *arg = expr->children->next;
            int num_args = 0;
            int arg_regs[100]; 
            ExprType arg_types[100]; 
            while (arg != NULL) {
                arg_regs[num_args] = codegen_expression(arg->node);
                if (arg_regs[num_args] == -2) arg_types[num_args] = TypeVoid; 
                else arg_types[num_args] = get_actual_type(arg->node);
                num_args++;
                arg = arg->next;
            }
            
            char mangled_name[256];
            strcpy(mangled_name, func_name); 
            if (global_root_ast != NULL) {
                struct node *exact_match = NULL;
                struct node *compat_match = NULL;
                struct node_list *child = global_root_ast->children;
                if (child) child = child->next;
                while (child) {
                    if (child->node->category == MethodDecl) {
                        struct node *m_header = child->node->children->node;
                        struct node *m_id = m_header->children->next->node;
                        if (strcmp(m_id->token, func_name) == 0) {
                            struct node *m_params = m_header->children->next->next->node;
                            struct node_list *p = m_params->children;
                            int p_count = 0, exact = 1, compat = 1;
                            while (p && p_count < num_args) {
                                int is_str_arr = (p->node->children->node->category == StringArray);
                                if (is_str_arr) {
                                    if (arg_regs[p_count] != -2) { exact=0; compat=0; }
                                } else {
                                    if (arg_regs[p_count] == -2) { exact=0; compat=0; }
                                    else {
                                        ExprType pt = get_expr_type(p->node->children->node->category);
                                        ExprType at = arg_types[p_count];
                                        if (pt != at) {
                                            exact = 0;
                                            if (!(pt == TypeDouble && at == TypeInt)) compat = 0;
                                        }
                                    }
                                }
                                p_count++;
                                p = p->next;
                            }
                            if (p == NULL && p_count == num_args) {
                                if (exact) exact_match = child->node;
                                else if (compat) compat_match = child->node;
                            }
                        }
                    }
                    child = child->next;
                }
                
                struct node *match = exact_match ? exact_match : compat_match;
                if (match) {
                    get_mangled_name(match, mangled_name);
                    struct node *m_params = match->children->node->children->next->next->node;
                    struct node_list *p = m_params->children;
                    int i = 0;
                    while (p && i < num_args) {
                        if (p->node->children->node->category != StringArray) {
                            ExprType pt = get_expr_type(p->node->children->node->category);
                            if (pt == TypeDouble && arg_types[i] == TypeInt) {
                                arg_regs[i] = promote_to_double(arg_regs[i], TypeInt);
                                arg_types[i] = TypeDouble;
                            }
                        }
                        p = p->next;
                        i++;
                    }
                } else {
                    sprintf(mangled_name, "m_%s", func_name);
                    for (int i=0; i<num_args; i++) {
                        if (arg_types[i] == TypeInt) strcat(mangled_name, "_i");
                        else if (arg_types[i] == TypeDouble) strcat(mangled_name, "_d");
                        else if (arg_types[i] == TypeBoolean) strcat(mangled_name, "_b");
                    }
                }
            }
            
            const char *ret_ll_type = get_llvm_type(expr->type);
            int tmp = -1;
            if (expr->type == TypeVoid) {
                printf("  call void @%s(", mangled_name); 
            } else {
                tmp = temporary++;
                printf("  %%%d = call %s @%s(", tmp, ret_ll_type, mangled_name);
            }
            for (int i = 0; i < num_args; i++) {
                if (i > 0) printf(", ");
                // A CURA FINAL DO RETURN_TYPES: Enviar argc e argv ao método certo
                if (arg_regs[i] == -2) {
                    printf("i32 %%argc, i8** %%argv");
                } else {
                    printf("%s %%%d", get_llvm_type(arg_types[i]), arg_regs[i]);
                }
            }
            printf(")\n");
            return tmp;
        }
        case Length: {
            int tmp = temporary++;
            printf("  %%%d = sub i32 %%argc, 1\n", tmp);
            return tmp;
        }
        case ParseArgs: {
            int index_reg = codegen_expression(expr->children->next->node);
            int c_index = temporary++;
            printf("  %%%d = add i32 %%%d, 1\n", c_index, index_reg); 
            int ptr_reg = temporary++;
            printf("  %%%d = getelementptr i8*, i8** %%argv, i32 %%%d\n", ptr_reg, c_index);
            int str_reg = temporary++;
            printf("  %%%d = load i8*, i8** %%%d\n", str_reg, ptr_reg);
            int res_reg = temporary++;
            
            struct node *id_node = expr->children->node;
            if (id_node && strstr(id_node->token, "Double")) {
                printf("  %%%d = call double @atof(i8* %%%d)\n", res_reg, str_reg);
            } else {
                printf("  %%%d = call i32 @atoi(i8* %%%d)\n", res_reg, str_reg);
            }
            return res_reg;
        }
        default:
            return -1; 
    }
}

void codegen_program(struct node *program) {
    if (!program) return;
    
    setlocale(LC_NUMERIC, "C"); 

    global_root_ast = program;
    declare_str_id = 0;
    use_str_id = 0;
    num_global_vars = 0;

    printf("declare i32 @printf(i8*, ...)\n");
    printf("declare i32 @atoi(i8*)\n");
    printf("declare double @atof(i8*)\n\n");
    
    printf("@.str.int = private unnamed_addr constant [3 x i8] c\"%%d\\00\"\n");
    printf("@.str.double = private unnamed_addr constant [6 x i8] c\"%%.16e\\00\"\n");
    printf("@.str.true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n");
    printf("@.str.false = private unnamed_addr constant [6 x i8] c\"false\\00\"\n");
    printf("@.str.str = private unnamed_addr constant [3 x i8] c\"%%s\\00\"\n\n");

    declare_global_strings(program);
    printf("\n");
    
    struct node_list *child = program->children;
    if (child) child = child->next; 
    
    struct node_list *iter = child;
    while (iter != NULL) {
        if (iter->node->category == FieldDecl) codegen_field(iter->node);
        iter = iter->next;
    }
    
    iter = child;
    while (iter != NULL) {
        if (iter->node->category == MethodDecl) codegen_method(iter->node);
        iter = iter->next;
    }
}

void codegen_method(struct node *method_node) {
    temporary = 1; 
    num_local_vars = 0;
    block_terminated = 0; 
    current_string_array_param = NULL;
    
    struct node *header = method_node->children->node;
    struct node *body = method_node->children->next->node;
    
    ExprType ret_type = get_expr_type(header->children->node->category);
    current_method_ret_type = ret_type;
    
    char mangled_name[256];
    get_mangled_name(method_node, mangled_name);
    current_method_name = mangled_name;
    
    struct node *params_node = header->children->next->next->node;
    
    if (strcmp(mangled_name, "main") == 0) {
        printf("define i32 @main(i32 %%argc, i8** %%argv) {\n");
    } else {
        printf("define %s @%s(", get_llvm_type(ret_type), mangled_name);
        struct node_list *param = params_node->children;
        int p_count = 0;
        while (param != NULL) {
            struct node *p_type_node = param->node->children->node;
            struct node *p_id_node = param->node->children->next->node;
            if (p_count > 0) printf(", ");
            if (p_type_node->category == StringArray) {
                printf("i32 %%argc, i8** %%argv");
                current_string_array_param = p_id_node->token;
            } else {
                printf("%s %%%s", get_llvm_type(get_expr_type(p_type_node->category)), p_id_node->token);
            }
            p_count++; param = param->next;
        }
        printf(") {\n");
    }
    
    struct node_list *param = params_node->children;
    while (param != NULL) {
        struct node *p_type_node = param->node->children->node;
        struct node *p_id_node = param->node->children->next->node;
        if (p_type_node->category == StringArray) {
            current_string_array_param = p_id_node->token;
        } else {
            int ptr_reg = temporary++;
            const char *ll_type = get_llvm_type(get_expr_type(p_type_node->category));
            printf("  %%%d = alloca %s\n", ptr_reg, ll_type);
            printf("  store %s %%%s, %s* %%%d\n", ll_type, p_id_node->token, ll_type, ptr_reg);
            local_vars[num_local_vars].name = p_id_node->token;
            local_vars[num_local_vars].reg_ptr = ptr_reg;
            local_vars[num_local_vars].type = get_expr_type(p_type_node->category);
            num_local_vars++;
        }
        param = param->next;
    }
    
    struct node_list *stmt = body->children;
    while (stmt != NULL) {
        codegen_statement(stmt->node);
        stmt = stmt->next;
    }

    if (!block_terminated) {
        if (strcmp(mangled_name, "main") == 0) printf("  ret i32 0\n"); 
        else if (ret_type == TypeVoid) printf("  ret void\n");
        else if (ret_type == TypeDouble) printf("  ret double 0.0\n");
        else printf("  ret %s 0\n", get_llvm_type(ret_type));
    }
    printf("}\n\n");
}