#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"

int temporary;   
int label_counter = 0; 

int declare_str_id = 0; 
int use_str_id = 0;     

// Estado para prevenir a impressão de instruções após um 'ret' ou 'br'
int block_terminated = 0;
char *current_method_name = NULL;

extern ExprType get_expr_type(enum category cat);
int codegen_expression(struct node *expr);

struct local_var_info {
    char *name;
    int reg_ptr;
    ExprType type;
};
struct local_var_info local_vars[1000];
int num_local_vars = 0;

int get_local_var(char *name) {
    for (int i = 0; i < num_local_vars; i++) {
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

// Devolve o tamanho real da string no LLVM
int get_llvm_string_length(const char *token) {
    int len = strlen(token);
    int start = (token[0] == '"') ? 1 : 0;
    int end = (token[len-1] == '"') ? len - 1 : len;
    int real_len = 0;
    for (int i = start; i < end; i++) {
        if (token[i] == '\\' && i + 1 < end) {
            i++; // salta o char escapado
        }
        real_len++;
    }
    return real_len + 1; // +1 para o \00
}

// Imprime a string convertendo \n para \0A (LLVM Hex format)
void print_llvm_string_literal(const char *token) {
    int len = strlen(token);
    int start = (token[0] == '"') ? 1 : 0;
    int end = (token[len-1] == '"') ? len - 1 : len;
    for (int i = start; i < end; i++) {
        if (token[i] == '\\' && i + 1 < end) {
            char next = token[i+1];
            if (next == 'n') { printf("\\0A"); i++; }
            else if (next == 't') { printf("\\09"); i++; }
            else if (next == 'r') { printf("\\0D"); i++; }
            else if (next == '\\') { printf("\\5C"); i++; }
            else if (next == '"') { printf("\\22"); i++; }
            else { printf("\\%02X", next); i++; }
        } else {
            printf("%c", token[i]);
        }
    }
    printf("\\00");
}

void codegen_method(struct node *method_node);

// Declarar globais (Fields)
void codegen_field(struct node *field_node) {
    struct node *type_node = field_node->children->node;
    struct node *id_node = field_node->children->next->node;
    const char *ll_type = get_llvm_type(get_expr_type(type_node->category));
    
    // O Juc exige que os atributos sejam iniciados com 0 (ou 0.0)
    if (get_expr_type(type_node->category) == TypeDouble) {
        printf("@%s = global %s 0.0\n", id_node->token, ll_type);
    } else {
        printf("@%s = global %s 0\n", id_node->token, ll_type);
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

// Funções auxiliares para gerir labels e terminadores
void emit_label(int label) {
    printf("L%d:\n", label);
    block_terminated = 0; // Um novo label significa que um novo bloco válido começou
}

void emit_br(int label) {
    if (!block_terminated) {
        printf("  br label %%L%d\n", label);
        block_terminated = 1;
    }
}

void codegen_statement(struct node *stmt) {
    // Se o bloco atual já teve um return/break, ignoramos o código morto seguinte!
    if (!stmt || block_terminated) return;
    
    if (stmt->category == Print) {
        struct node *expr = stmt->children->node;
        
        if (expr->category != StrLit) {
            int reg = codegen_expression(expr);
            
            if (expr->type == TypeInt) {
                int ptr = temporary++;
                int call_reg = temporary++; 
                printf("  %%%d = getelementptr [4 x i8], [4 x i8]* @.str.int, i32 0, i32 0\n", ptr);
                printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d, i32 %%%d)\n", call_reg, ptr, reg);
            } 
            else if (expr->type == TypeDouble) {
                int ptr = temporary++;
                int call_reg = temporary++; 
                printf("  %%%d = getelementptr [7 x i8], [7 x i8]* @.str.double, i32 0, i32 0\n", ptr);
                printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d, double %%%d)\n", call_reg, ptr, reg);
            } 
            else if (expr->type == TypeBoolean) {
                int ptr_true = temporary++;
                printf("  %%%d = getelementptr [6 x i8], [6 x i8]* @.str.true, i32 0, i32 0\n", ptr_true);
                int ptr_false = temporary++;
                printf("  %%%d = getelementptr [7 x i8], [7 x i8]* @.str.false, i32 0, i32 0\n", ptr_false);
                int str_ptr = temporary++;
                printf("  %%%d = select i1 %%%d, i8* %%%d, i8* %%%d\n", str_ptr, reg, ptr_true, ptr_false);
                int call_reg = temporary++;
                printf("  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)\n", call_reg, str_ptr);
            }
        }
        else { // StrLit
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
        codegen_statement(stmt->children->next->node);
        emit_br(end_block);
        
        emit_label(else_block);
        if (stmt->children->next->next) { 
            codegen_statement(stmt->children->next->next->node);
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
        codegen_statement(stmt->children->next->node);
        emit_br(cond_block);
        
        emit_label(end_block);
    }
    else if (stmt->category == Return) {
        if (!block_terminated) {
            if (stmt->children && stmt->children->node) {
                int ret_val = codegen_expression(stmt->children->node);
                printf("  ret %s %%%d\n", get_llvm_type(stmt->children->node->type), ret_val);
            } else {
                // A main exige um i32 nativamente!
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
        struct node_list *child = stmt->children;
        while (child != NULL) {
            codegen_statement(child->node);
            child = child->next;
        }
    }
    else if (stmt->category != VarDecl) {
        codegen_expression(stmt);
    }
}

int codegen_expression(struct node *expr) {
    if (!expr) return -1;
    
    switch (expr->category) {
        case Natural: {
            int tmp = temporary++;
            printf("  %%%d = add i32 0, %s\n", tmp, expr->token);
            return tmp;
        }
        case Decimal: {
            int tmp = temporary++;
            printf("  %%%d = fadd double 0.0, %s\n", tmp, expr->token);
            return tmp;
        }
        case BoolLit: {
            int tmp = temporary++;
            printf("  %%%d = add i1 0, %d\n", tmp, strcmp(expr->token, "true") == 0 ? 1 : 0);
            return tmp;
        }
        case Identifier: {
            int ptr = get_local_var(expr->token);
            const char *ll_type = get_llvm_type(expr->type);
            int tmp = temporary++; 
            if (ptr != -1) {
                printf("  %%%d = load %s, %s* %%%d\n", tmp, ll_type, ll_type, ptr);
            } else { // Se for -1, é uma variável global!
                printf("  %%%d = load %s, %s* @%s\n", tmp, ll_type, ll_type, expr->token);
            }
            return tmp;
        }
        case Assign: {
            struct node *id_node = expr->children->node;
            struct node *val_node = expr->children->next->node;
            
            int val_reg = codegen_expression(val_node);
            
            int ptr = get_local_var(id_node->token);
            const char *ll_type = get_llvm_type(id_node->type);
            
            if (ptr != -1) {
                printf("  store %s %%%d, %s* %%%d\n", ll_type, val_reg, ll_type, ptr);
            } else { // Global
                printf("  store %s %%%d, %s* @%s\n", ll_type, val_reg, ll_type, id_node->token);
            }
            return val_reg; 
        }
        case Add: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++; 
            if (expr->type == TypeDouble) printf("  %%%d = fadd double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = add i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Sub: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->type == TypeDouble) printf("  %%%d = fsub double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = sub i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Mul: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->type == TypeDouble) printf("  %%%d = fmul double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = mul i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Div: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->type == TypeDouble) printf("  %%%d = fdiv double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = sdiv i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Mod: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->type == TypeDouble) printf("  %%%d = frem double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = srem i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Plus: {
            return codegen_expression(expr->children->node);
        }
        case Minus: {
            int e1 = codegen_expression(expr->children->node);
            int tmp = temporary++;
            if (expr->type == TypeDouble) printf("  %%%d = fsub double 0.0, %%%d\n", tmp, e1);
            else printf("  %%%d = sub i32 0, %%%d\n", tmp, e1);
            return tmp;
        }
        case Not: {
            int e1 = codegen_expression(expr->children->node);
            int tmp = temporary++;
            printf("  %%%d = xor i1 %%%d, 1\n", tmp, e1);
            return tmp;
        }
        case Eq: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->children->node->type == TypeDouble) printf("  %%%d = fcmp oeq double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = icmp eq i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Ne: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->children->node->type == TypeDouble) printf("  %%%d = fcmp one double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = icmp ne i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Lt: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->children->node->type == TypeDouble) printf("  %%%d = fcmp olt double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = icmp slt i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Le: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->children->node->type == TypeDouble) printf("  %%%d = fcmp ole double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = icmp sle i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Gt: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->children->node->type == TypeDouble) printf("  %%%d = fcmp ogt double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = icmp sgt i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Ge: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            if (expr->children->node->type == TypeDouble) printf("  %%%d = fcmp oge double %%%d, %%%d\n", tmp, e1, e2);
            else printf("  %%%d = icmp sge i32 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case And: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            printf("  %%%d = and i1 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Or: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            printf("  %%%d = or i1 %%%d, %%%d\n", tmp, e1, e2);
            return tmp;
        }
        case Xor: {
            int e1 = codegen_expression(expr->children->node);
            int e2 = codegen_expression(expr->children->next->node);
            int tmp = temporary++;
            printf("  %%%d = xor i1 %%%d, %%%d\n", tmp, e1, e2);
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
                arg_types[num_args] = arg->node->type;
                num_args++;
                arg = arg->next;
            }
            
            const char *ret_ll_type = get_llvm_type(expr->type);
            int tmp = -1;
            
            if (expr->type == TypeVoid) {
                printf("  call void @%s(", func_name); 
            } else {
                tmp = temporary++;
                printf("  %%%d = call %s @%s(", tmp, ret_ll_type, func_name);
            }
            
            for (int i = 0; i < num_args; i++) {
                if (i > 0) printf(", ");
                printf("%s %%%d", get_llvm_type(arg_types[i]), arg_regs[i]);
            }
            printf(")\n");
            
            return tmp;
        }
        case Length: {
            int tmp = temporary++;
            printf("  %%%d = sub i32 %%argc, 1\n", tmp); // argc real conta com o nome do executável
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
            // Se for Double.parseDouble, usamos a função atof!
            if (expr->type == TypeDouble) {
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

    declare_str_id = 0;
    use_str_id = 0;

    printf("declare i32 @printf(i8*, ...)\n");
    printf("declare i32 @atoi(i8*)\n\n");
    printf("declare double @atof(i8*)\n\n");
    
    printf("@.str.int = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\"\n");
    printf("@.str.double = private unnamed_addr constant [7 x i8] c\"%%.16e\\0A\\00\"\n");
    printf("@.str.true = private unnamed_addr constant [6 x i8] c\"true\\0A\\00\"\n");
    printf("@.str.false = private unnamed_addr constant [7 x i8] c\"false\\0A\\00\"\n");
    printf("@.str.str = private unnamed_addr constant [4 x i8] c\"%%s\\0A\\00\"\n\n");

    declare_global_strings(program);
    printf("\n");
    
    struct node_list *child = program->children;
    if (child) child = child->next; 

    while (child != NULL) {
        if (child->node->category == MethodDecl) {
            codegen_method(child->node);
        } else if (child->node->category == FieldDecl) {
            codegen_field(child->node);
        }
        child = child->next;
    }
}

void codegen_method(struct node *method_node) {
    temporary = 1; 
    num_local_vars = 0;
    block_terminated = 0; // Reset para o novo método
    
    struct node *header = method_node->children->node;
    struct node *body = method_node->children->next->node;
    
    ExprType ret_type = get_expr_type(header->children->node->category);
    struct node *id_node = header->children->next->node;
    char *func_name = id_node->token;
    current_method_name = func_name; // Guardar para saber se estamos na main
    struct node *params_node = header->children->next->next->node;
    
    if (strcmp(func_name, "main") == 0) {
        printf("define i32 @main(i32 %%argc, i8** %%argv) {\n");
    } else {
        printf("define %s @%s(", get_llvm_type(ret_type), func_name);
        struct node_list *param = params_node->children;
        int p_count = 0;
        while (param != NULL) {
            struct node *p_type_node = param->node->children->node;
            struct node *p_id_node = param->node->children->next->node;
            if (p_count > 0) printf(", ");
            printf("%s %%%s", get_llvm_type(get_expr_type(p_type_node->category)), p_id_node->token);
            p_count++; param = param->next;
        }
        printf(") {\n");
    }
    
    struct node_list *param = params_node->children;
    while (param != NULL) {
        struct node *p_type_node = param->node->children->node;
        struct node *p_id_node = param->node->children->next->node;
        
        if (p_type_node->category != StringArray) {
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
        if (stmt->node->category == VarDecl) {
            struct node *v_type_node = stmt->node->children->node;
            struct node *v_id_node = stmt->node->children->next->node;
            
            int ptr_reg = temporary++;
            const char *ll_type = get_llvm_type(get_expr_type(v_type_node->category));
            
            printf("  %%%d = alloca %s\n", ptr_reg, ll_type);
            
            local_vars[num_local_vars].name = v_id_node->token;
            local_vars[num_local_vars].reg_ptr = ptr_reg;
            local_vars[num_local_vars].type = get_expr_type(v_type_node->category);
            num_local_vars++;
        }
        stmt = stmt->next;
    }
    
    stmt = body->children;
    while (stmt != NULL) {
        if (stmt->node->category != VarDecl) {
            codegen_statement(stmt->node);
        }
        stmt = stmt->next;
    }

    if (!block_terminated) {
        if (strcmp(func_name, "main") == 0) {
            printf("  ret i32 0\n"); 
        } else if (ret_type == TypeVoid) {
            printf("  ret void\n");
        } else if (ret_type == TypeDouble) {
            printf("  ret double 0.0\n");
        } else {
            printf("  ret %s 0\n", get_llvm_type(ret_type));
        }
    }
    
    printf("}\n\n");
}