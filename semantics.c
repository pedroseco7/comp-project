#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"

struct symbol_table *sym_tables = NULL;
int semantic_errors = 0;

void strip_underscores(char *str, char *dest) {
    int i = 0, j = 0;
    while (str[i]) {
        if (str[i] != '_') dest[j++] = str[i];
        i++;
    }
    dest[j] = '\0';
}

const char* get_type_name(ExprType type) {
    switch(type) {
        case TypeInt: return "int";
        case TypeDouble: return "double";
        case TypeBoolean: return "boolean";
        case TypeStringArray: return "String[]";
        case TypeVoid: return "void";
        case TypeUndef: return "undef";
        case TypeString: return "String";
        default: return "";
    }
}

ExprType get_expr_type(enum category cat) {
    switch(cat) {
        case Int: return TypeInt;
        case Double: return TypeDouble;
        case Bool: return TypeBoolean;
        case StringArray: return TypeStringArray;
        case Void: return TypeVoid;
        default: return TypeNone;
    }
}

struct symbol_table *create_table(char *name, char *type) {
    struct symbol_table *new_t = malloc(sizeof(struct symbol_table));
    new_t->table_name = strdup(name);
    new_t->table_type = strdup(type);
    new_t->first_symbol = NULL;
    new_t->next = NULL;
    return new_t;
}

void add_table(struct symbol_table *new_table) {
    if (sym_tables == NULL) { sym_tables = new_table; return; }
    struct symbol_table *curr = sym_tables;
    while (curr->next != NULL) curr = curr->next;
    curr->next = new_table;
}

struct symbol *insert_symbol(struct symbol_table *table, char *name, ExprType type, int is_param, int line, int col) {
    struct symbol *new_s = malloc(sizeof(struct symbol));
    new_s->name = strdup(name);
    new_s->type = type;
    new_s->param_types = NULL;
    new_s->num_params = 0;
    new_s->is_param = is_param;
    new_s->is_method = 0;
    new_s->line = line;
    new_s->col = col;
    new_s->next = NULL;

    if (table->first_symbol == NULL) table->first_symbol = new_s;
    else {
        struct symbol *curr = table->first_symbol;
        while (curr->next != NULL) curr = curr->next;
        curr->next = new_s;
    }
    return new_s;
}

void check_and_insert(struct symbol_table *table, struct node *id_node, ExprType type, int is_param) {
    char *name = id_node->token;
    if (strcmp(name, "_") == 0) {
        printf("Line %d, col %d: Symbol _ is reserved\n", id_node->line, id_node->col);
        semantic_errors++; return;
    }
    struct symbol *curr = table->first_symbol;
    while (curr != NULL) {
        /* A magia está aqui: !curr->is_method */
        if (!curr->is_method && strcmp(curr->name, name) == 0) {
            printf("Line %d, col %d: Symbol %s already defined\n", id_node->line, id_node->col, name);
            semantic_errors++; return;
        }
        curr = curr->next;
    }
    insert_symbol(table, name, type, is_param, id_node->line, id_node->col);
}

struct symbol *lookup_symbol(struct symbol_table *local_table, struct symbol_table *global_table, char *name, int use_line, int use_col) {
    struct symbol *sym = local_table ? local_table->first_symbol : NULL;
    while (sym != NULL) {
        /* A magia está aqui: !sym->is_method */
        if (!sym->is_method && strcmp(sym->name, name) == 0) {
            if (sym->is_param || sym->line < use_line || (sym->line == use_line && sym->col < use_col) || sym->line == 0) return sym;
        }
        sym = sym->next;
    }
    sym = global_table ? global_table->first_symbol : NULL;
    while (sym != NULL) {
        /* A magia está aqui: !sym->is_method */
        if (!sym->is_method && strcmp(sym->name, name) == 0) return sym;
        sym = sym->next;
    }
    return NULL;
}

int is_method_redefined(struct symbol_table *global, char *name, ExprType *types, int num_params) {
    struct symbol *s = global->first_symbol;
    while(s) {
        if (s->is_method && strcmp(s->name, name) == 0 && s->num_params == num_params) {
            int match = 1;
            for(int i = 0; i < num_params; i++) {
                if(s->param_types[i] != types[i]) match = 0;
            }
            if(match) return 1;
        }
        s = s->next;
    }
    return 0;
}

void build_symbol_tables(struct node *ast_root) {
    if (ast_root == NULL || ast_root->category != Program) return;
    struct node_list *child = ast_root->children;
    char *class_name = child->node->token;
    struct symbol_table *global_table = create_table(class_name, "Class");
    add_table(global_table);
    
    child = child->next;
    while (child != NULL) {
        struct node *decl = child->node;
        if (decl->category == FieldDecl) {
            check_and_insert(global_table, decl->children->next->node, get_expr_type(decl->children->node->category), 0); 
        }
        else if (decl->category == MethodDecl) {
            struct node *header = decl->children->node;
            struct node *id_node = header->children->next->node;
            struct node *params_node = header->children->next->next->node;
            
            ExprType p_types[2048];
            int p_count = 0;
            struct node_list *p = params_node->children;
            while(p) {
                p_types[p_count++] = get_expr_type(p->node->children->node->category);
                p = p->next;
            }

            if (is_method_redefined(global_table, id_node->token, p_types, p_count)) {
                /* 1. Verificar parametros repetidos PRIMEIRO */
                struct symbol_table *dummy = create_table("dummy", "Method");
                struct node_list *p2 = params_node->children;
                while(p2) { 
                    check_and_insert(dummy, p2->node->children->next->node, get_expr_type(p2->node->children->node->category), 1);
                    p2 = p2->next; 
                }

                /* 2. Imprimir erro do metodo DEPOIS */
                printf("Line %d, col %d: Symbol %s(", id_node->line, id_node->col, id_node->token);
                for(int i = 0; i < p_count; i++) {
                    printf("%s%s", get_type_name(p_types[i]), (i == p_count - 1) ? "" : ",");
                }
                printf(") already defined\n");
                semantic_errors++;
                decl->is_duplicate = 1; 
            } else {
                struct symbol *ms = insert_symbol(global_table, id_node->token, get_expr_type(header->children->node->category), 0, id_node->line, id_node->col);
                ms->is_method = 1;
                ms->num_params = p_count;
                ms->param_types = malloc(sizeof(ExprType) * p_count);
                for(int i = 0; i < p_count; i++) ms->param_types[i] = p_types[i];

                char sig[4096]; sprintf(sig, "%s(", id_node->token);
                for(int i = 0; i < p_count; i++) {
                    strcat(sig, get_type_name(p_types[i]));
                    if(i < p_count - 1) strcat(sig, ",");
                }
                strcat(sig, ")");
                
                struct symbol_table *mt = create_table(sig, "Method"); 
                add_table(mt);
                insert_symbol(mt, "return", get_expr_type(header->children->node->category), 0, 0, 0);
                
                p = params_node->children;
                while(p) { 
                    check_and_insert(mt, p->node->children->next->node, get_expr_type(p->node->children->node->category), 1);
                    p = p->next; 
                }
            }
        }
        child = child->next;
    }
}

void print_symbol_tables() {
    struct symbol_table *curr_table = sym_tables;
    while (curr_table != NULL) {
        printf("===== %s %s Symbol Table =====\n", curr_table->table_type, curr_table->table_name);
        struct symbol *curr_sym = curr_table->first_symbol;
        while (curr_sym != NULL) {
            printf("%s\t", curr_sym->name);
            if (curr_sym->is_method) {
                printf("(");
                for (int i = 0; i < curr_sym->num_params; i++) {
                    printf("%s", get_type_name(curr_sym->param_types[i]));
                    if (i < curr_sym->num_params - 1) printf(",");
                }
                printf(")\t");
            } else printf("\t");
            printf("%s", get_type_name(curr_sym->type));
            if (curr_sym->is_param) printf("\tparam");
            printf("\n");
            curr_sym = curr_sym->next;
        }
        printf("\n");
        curr_table = curr_table->next;
    }
}

void annotate_ast(struct node *node, struct symbol_table *method_table) {
    if (node == NULL || node->is_duplicate) return;
    
    if (node->category == MethodDecl) {
        struct node *header = node->children->node;
        char sig[4096]; sprintf(sig, "%s(", header->children->next->node->token);
        struct node_list *p = header->children->next->next->node->children;
        while(p) {
            strcat(sig, get_type_name(get_expr_type(p->node->children->node->category)));
            if(p->next) strcat(sig, ",");
            p = p->next;
        }
        strcat(sig, ")");
        
        struct symbol_table *curr = sym_tables;
        while (curr) { 
            if (strcmp(curr->table_name, sig) == 0) { 
                method_table = curr; 
                break; 
            } 
            curr = curr->next; 
        }
    }
    
    struct node_list *child = node->children;
    while (child != NULL) {
        if (!(node->category == Call && child == node->children)) 
            annotate_ast(child->node, method_table);
        child = child->next;
    }
    
    switch (node->category) {
        case VarDecl:
            if (method_table) {
                check_and_insert(method_table, node->children->next->node, get_expr_type(node->children->node->category), 0);
            }
            break;
        case Identifier:
            if (method_table && node->is_expr) {
                struct symbol *s = lookup_symbol(method_table, sym_tables, node->token, node->line, node->col);
                if (s && !s->is_method) node->type = s->type;
                else { node->type = TypeUndef; printf("Line %d, col %d: Cannot find symbol %s\n", node->line, node->col, node->token); semantic_errors++; }
            }
            break;
        case Natural:
            node->type = TypeInt;
            {
                char clean[2048]; strip_underscores(node->token, clean);
                if (atof(clean) > 2147483647.0) { printf("Line %d, col %d: Number %s out of bounds\n", node->line, node->col, node->token); semantic_errors++; }
            }
            break;
        case Decimal:
            node->type = TypeDouble;
            {
                char clean[2048]; strip_underscores(node->token, clean);
                double val = atof(clean);
                
                /* Se atof der 0.0, mas o número no código fonte NÃO ERA zero */
                if (val == 0.0) {
                    int is_real_zero = 1;
                    for (int i = 0; clean[i]; i++) {
                        /* Se encontrar um digito de 1 a 9 antes de qualquer 'e' ou 'E' */
                        if (clean[i] >= '1' && clean[i] <= '9') {
                            is_real_zero = 0;
                            break;
                        }
                    }
                    if (!is_real_zero) {
                        printf("Line %d, col %d: Number %s out of bounds\n", node->line, node->col, node->token);
                        semantic_errors++;
                    }
                }
                /* Se for Infinito (overflow) */
                // else if (val * 0.0 != 0.0) { 
                //    printf("Line %d, col %d: Number %s out of bounds\n", node->line, node->col, node->token);
                //    semantic_errors++;
                // }
            }
            break;
        case BoolLit: node->type = TypeBoolean; break;
        case Add: case Sub: case Mul: case Div: case Mod:
            node->is_expr = 1;
            {
                ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
                if (t1 == TypeBoolean || t2 == TypeBoolean || t1 == TypeVoid || t2 == TypeVoid || t1 == TypeStringArray || t2 == TypeStringArray || t1 == TypeUndef || t2 == TypeUndef) {
                    node->type = TypeUndef;
                    char op = (node->category == Add) ? '+' : (node->category == Sub) ? '-' : (node->category == Mul) ? '*' : (node->category == Div) ? '/' : '%';
                    printf("Line %d, col %d: Operator %c cannot be applied to types %s, %s\n", node->line, node->col, op, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                } else node->type = (t1 == TypeDouble || t2 == TypeDouble) ? TypeDouble : TypeInt;
            }
            break;
        case Assign:
            node->is_expr = 1;
            {
                ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
                node->type = t1;
                if (t1 == TypeStringArray || t2 == TypeStringArray || (t1 != t2 && !(t1 == TypeDouble && t2 == TypeInt))) {
                    printf("Line %d, col %d: Operator = cannot be applied to types %s, %s\n", node->line, node->col, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                }
            }
            break;
        case Call:
            node->is_expr = 1;
            {
                struct node *id_node = node->children->node; 
                id_node->is_expr = 1;
                struct node_list *arg = node->children->next; 
                int count = 0; 
                ExprType at[2048];
                char call_sig[4096]; 
                sprintf(call_sig, "%s(", id_node->token);
                
                while(arg) { 
                    at[count] = arg->node->type; 
                    strcat(call_sig, get_type_name(at[count])); 
                    if(arg->next) strcat(call_sig, ","); 
                    arg = arg->next; 
                    count++; 
                }
                strcat(call_sig, ")");
                
                struct symbol *best = NULL; 
                struct symbol *curr = sym_tables->first_symbol;
                
                /* PASSADA 1: Procura Match Exato (Prioridade Maxima) */
                while(curr) {
                    if (curr->is_method && strcmp(curr->name, id_node->token) == 0 && curr->num_params == count) {
                        int match = 1; 
                        for(int i = 0; i < count; i++) {
                            if(curr->param_types[i] != at[i]) {
                                match = 0; 
                                break;
                            }
                        }
                        if(match) { 
                            best = curr; 
                            break; 
                        }
                    }
                    curr = curr->next;
                }
                
                /* PASSADA 2: Se não encontrou exato, procura Widening (int -> double) */
                int match_count = 0;
                struct symbol *ambiguous_best = NULL;

                if (!best) {
                    curr = sym_tables->first_symbol;
                    while(curr) {
                        if (curr->is_method && strcmp(curr->name, id_node->token) == 0 && curr->num_params == count) {
                            int match = 1; 
                            for(int i = 0; i < count; i++) {
                                ExprType param = curr->param_types[i];
                                if (!(param == at[i] || (param == TypeDouble && at[i] == TypeInt))) {
                                    match = 0; 
                                    break;
                                }
                            }
                            if(match) { 
                                ambiguous_best = curr;
                                match_count++; /* Conta em vez de dar break imediatamente */
                            }
                        }
                        curr = curr->next;
                    }
                    if (match_count == 1) {
                        best = ambiguous_best;
                    }
                }
                
                if (best) {
                    node->type = best->type; 
                    char rs[4096] = "(";
                    for(int i = 0; i < best->num_params; i++) { 
                        strcat(rs, get_type_name(best->param_types[i])); 
                        if(i < best->num_params - 1) strcat(rs, ","); 
                    }
                    strcat(rs, ")"); 
                    id_node->func_sig = strdup(rs);
                } else if (match_count > 1) {
                    /* AMBIGUIDADE DETETADA */
                    node->type = TypeUndef; 
                    id_node->func_sig = strdup("undef");
                    printf("Line %d, col %d: Reference to method %s is ambiguous\n", id_node->line, id_node->col, call_sig); 
                    semantic_errors++;
                } else {
                    /* NAO ENCONTROU NADA */
                    node->type = TypeUndef; 
                    id_node->func_sig = strdup("undef");
                    printf("Line %d, col %d: Cannot find symbol %s\n", id_node->line, id_node->col, call_sig); 
                    semantic_errors++;
                }
            }
            break;
        case Return:
            if (method_table) {
                struct symbol *ret = lookup_symbol(method_table, NULL, "return", 0, 0);
                ExprType exp = ret ? ret->type : TypeVoid;
                if (node->children == NULL) { if (exp != TypeVoid) { printf("Line %d, col %d: Incompatible type void in return statement\n", node->line, node->col); semantic_errors++; } }
                else { 
                    ExprType act = node->children->node->type;
                    if (exp == TypeVoid || (exp != act && !(exp == TypeDouble && act == TypeInt))) { printf("Line %d, col %d: Incompatible type %s in return statement\n", node->children->node->line, node->children->node->col, get_type_name(act)); semantic_errors++; }
                }
            }
            break;
        case Print:
            if (node->children->node->category != StrLit && (node->children->node->type == TypeVoid || node->children->node->type == TypeStringArray || node->children->node->type == TypeUndef)) {
                printf("Line %d, col %d: Incompatible type %s in System.out.print statement\n", node->children->node->line, node->children->node->col, get_type_name(node->children->node->type));
                semantic_errors++;
            }
            break;
        case ParseArgs:
            node->type = TypeInt; node->is_expr = 1;
            if (node->children->node->type != TypeStringArray || node->children->next->node->type != TypeInt) {
                printf("Line %d, col %d: Operator Integer.parseInt cannot be applied to types %s, %s\n", node->line, node->col, get_type_name(node->children->node->type), get_type_name(node->children->next->node->type));
                semantic_errors++;
            }
            break;
        case Length:
            node->type = TypeInt; node->is_expr = 1;
            if (node->children->node->type != TypeStringArray) { printf("Line %d, col %d: Operator .length cannot be applied to type %s\n", node->line, node->col, get_type_name(node->children->node->type)); semantic_errors++; }
            break;
        case And: case Or:
            node->is_expr = 1; node->type = TypeBoolean;
            { ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              if (t1 != TypeBoolean || t2 != TypeBoolean) { printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, (node->category == And ? "&&" : "||"), get_type_name(t1), get_type_name(t2)); semantic_errors++; }
            }
            break;
        case Xor:
            node->is_expr = 1;
            {
                ExprType t1 = node->children->node->type;
                ExprType t2 = node->children->next->node->type;
                if (t1 == TypeInt && t2 == TypeInt) {
                    node->type = TypeInt;
                } else {
                    node->type = TypeInt; 
                    printf("Line %d, col %d: Operator ^ cannot be applied to types %s, %s\n", 
                           node->line, node->col, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                }
            }
            break;
        case Lshift: case Rshift:
            node->is_expr = 1; node->type = TypeInt;
            { ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              if (t1 != TypeInt || t2 != TypeInt) { printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, (node->category == Lshift ? "<<" : ">>"), get_type_name(t1), get_type_name(t2)); semantic_errors++; }
            }
            break;
        case Not:
            node->is_expr = 1; node->type = TypeBoolean;
            if (node->children->node->type != TypeBoolean) { printf("Line %d, col %d: Operator ! cannot be applied to type %s\n", node->line, node->col, get_type_name(node->children->node->type)); semantic_errors++; }
            break;
        case Eq: case Ne:
            node->is_expr = 1; node->type = TypeBoolean;
            { ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              int v = ((t1 == TypeBoolean && t2 == TypeBoolean) || ((t1 == TypeInt || t1 == TypeDouble) && (t2 == TypeInt || t2 == TypeDouble)));
              if (!v) { printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, (node->category == Eq ? "==" : "!="), get_type_name(t1), get_type_name(t2)); semantic_errors++; }
            }
            break;
        case Lt: case Gt: case Le: case Ge:
            node->is_expr = 1; node->type = TypeBoolean;
            { ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              if (!((t1 == TypeInt || t1 == TypeDouble) && (t2 == TypeInt || t2 == TypeDouble))) {
                  char *op = "<"; if(node->category == Gt) op = ">"; else if(node->category == Le) op = "<="; else if(node->category == Ge) op = ">=";
                  printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, op, get_type_name(t1), get_type_name(t2)); semantic_errors++;
              }
            }
            break;
        case Plus: case Minus:
            node->is_expr = 1;
            { ExprType t1 = node->children->node->type;
              if (t1 == TypeInt || t1 == TypeDouble) node->type = t1;
              else { node->type = TypeUndef; printf("Line %d, col %d: Operator %s cannot be applied to type %s\n", node->line, node->col, (node->category == Plus ? "+" : "-"), get_type_name(t1)); semantic_errors++; }
            }
            break;
        case If: case While:
            if (node->children->node->type != TypeBoolean) { printf("Line %d, col %d: Incompatible type %s in %s statement\n", node->children->node->line, node->children->node->col, get_type_name(node->children->node->type), (node->category == If ? "if" : "while")); semantic_errors++; }
            break;

        case StrLit:
            node->type = TypeString;
            node->is_expr = 1;
            break;
        default: break;
    }
}