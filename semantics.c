#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"

#define HASH_SIZE 65537
#define MAX_PARAMS 100000

struct symbol_table *sym_tables = NULL;
struct symbol_table *last_sym_table = NULL;
int semantic_errors = 0;

ExprType global_p_types[MAX_PARAMS];
ExprType global_at[MAX_PARAMS];

/* Cache Hash Map O(1) para Símbolos - VELOCIDADE MÁXIMA */
struct symbol_cache_node {
    struct symbol_table *table;
    struct symbol *sym;
    struct symbol_cache_node *next;
};
struct symbol_cache_node *sym_cache[HASH_SIZE];

/* Cache Hash Map O(1) para Tabelas - ELIMINA O TLE NO MOOSHAK! */
struct table_cache_node {
    struct symbol_table *table;
    struct table_cache_node *next;
};
struct table_cache_node *table_cache_map[HASH_SIZE];

unsigned long hash_str(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) h = ((h << 5) + h) + c;
    return h % HASH_SIZE;
}

void add_to_cache(struct symbol_table *table, struct symbol *sym) {
    if (!sym || !sym->name) return;
    unsigned long h = hash_str(sym->name);
    struct symbol_cache_node *n = malloc(sizeof(struct symbol_cache_node));
    n->table = table;
    n->sym = sym;
    n->next = sym_cache[h];
    sym_cache[h] = n;
}

void remove_from_cache(struct symbol_table *table, struct symbol *sym) {
    if (!sym || !sym->name) return;
    unsigned long h = hash_str(sym->name);
    struct symbol_cache_node *curr = sym_cache[h];
    struct symbol_cache_node *prev = NULL;
    while (curr != NULL) {
        if (curr->table == table && curr->sym == sym) {
            if (prev == NULL) sym_cache[h] = curr->next;
            else prev->next = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void add_table_to_cache(struct symbol_table *t) {
    if (!t || !t->table_name) return;
    unsigned long h = hash_str(t->table_name);
    struct table_cache_node *n = malloc(sizeof(struct table_cache_node));
    n->table = t;
    n->next = table_cache_map[h];
    table_cache_map[h] = n;
}

struct symbol_table *get_table_from_cache(char *name) {
    if (!name) return NULL;
    unsigned long h = hash_str(name);
    struct table_cache_node *curr = table_cache_map[h];
    while(curr) {
        if (strcmp(curr->table->table_name, name) == 0) return curr->table;
        curr = curr->next;
    }
    return NULL;
}

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
    new_t->last_symbol = NULL;
    new_t->next = NULL;
    return new_t;
}

void add_table(struct symbol_table *new_table) {
    if (sym_tables == NULL) { 
        sym_tables = new_table; 
        last_sym_table = new_table;
    } else {
        last_sym_table->next = new_table;
        last_sym_table = new_table;
    }
    add_table_to_cache(new_table); 
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

    if (table->first_symbol == NULL) {
        table->first_symbol = new_s;
        table->last_symbol = new_s;
    } else {
        table->last_symbol->next = new_s;
        table->last_symbol = new_s;
    }
    
    /* FIX DE PERFORMANCE: Tudo vai para a cache para mantermos pesquisa O(1) */
    add_to_cache(table, new_s);
    return new_s;
}

void check_and_insert(struct symbol_table *table, struct node *id_node, ExprType type, int is_param) {
    if (!id_node || !id_node->token) return;
    char *name = id_node->token;
    if (strcmp(name, "_") == 0) {
        printf("Line %d, col %d: Symbol _ is reserved\n", id_node->line, id_node->col);
        semantic_errors++; return;
    }
    
    /* O(1) Lookup: O Mooshak não nos vence pelo cansaço! */
    unsigned long h = hash_str(name);
    struct symbol_cache_node *curr = sym_cache[h];
    while (curr != NULL) {
        if (curr->table == table && !curr->sym->is_method && strcmp(curr->sym->name, name) == 0) {
            printf("Line %d, col %d: Symbol %s already defined\n", id_node->line, id_node->col, name);
            semantic_errors++; return;
        }
        curr = curr->next;
    }
    
    insert_symbol(table, name, type, is_param, id_node->line, id_node->col);
}

struct symbol *lookup_symbol(struct symbol_table *local_table, struct symbol_table *global_table, char *name, int use_line, int use_col) {
    if (!name) return NULL;
    
    unsigned long h = hash_str(name);
    struct symbol_cache_node *curr = sym_cache[h];
    struct symbol *best_local = NULL;
    struct symbol *best_global = NULL;
    
    while (curr != NULL) {
        if (!curr->sym->is_method && strcmp(curr->sym->name, name) == 0) {
            if (curr->table == local_table) {
                if (curr->sym->is_param || curr->sym->line < use_line || (curr->sym->line == use_line && curr->sym->col < use_col) || curr->sym->line == 0) {
                    best_local = curr->sym;
                }
            } else if (curr->table == global_table) {
                best_global = curr->sym;
            }
        }
        curr = curr->next;
    }
    
    if (best_local) return best_local;
    return best_global;
}

int is_method_redefined(struct symbol_table *global, char *name, ExprType *types, int num_params) {
    unsigned long h = hash_str(name);
    struct symbol_cache_node *curr = sym_cache[h];
    while (curr != NULL) {
        if (curr->table == global && curr->sym->is_method && strcmp(curr->sym->name, name) == 0 && curr->sym->num_params == num_params) {
            int match = 1;
            for(int i = 0; i < num_params; i++) {
                if(curr->sym->param_types[i] != types[i]) { match = 0; break; }
            }
            if(match) return 1;
        }
        curr = curr->next;
    }
    return 0;
}

void build_symbol_tables(struct node *ast_root) {
    if (ast_root == NULL || ast_root->category != Program) return;
    struct node_list *child = ast_root->children;
    if (!child || !child->node) return;
    char *class_name = child->node->token;
    struct symbol_table *global_table = create_table(class_name, "Class");
    add_table(global_table);
    
    child = child->next;
    while (child != NULL) {
        struct node *decl = child->node;
        if (!decl) { child = child->next; continue; }
        
        if (decl->category == FieldDecl) {
            if (decl->children && decl->children->next && decl->children->next->node) {
                check_and_insert(global_table, decl->children->next->node, get_expr_type(decl->children->node->category), 0); 
            }
        }
        else if (decl->category == MethodDecl) {
            struct node *header = decl->children->node;
            struct node *id_node = header->children->next->node;
            struct node *params_node = header->children->next->next->node;
            
            int p_count = 0;
            struct node_list *p = params_node->children;
            while(p) { p_count++; p = p->next; }
            
            ExprType *p_types = malloc(sizeof(ExprType) * (p_count > 0 ? p_count : 1));
            int idx = 0;
            p = params_node->children;
            while(p) { p_types[idx++] = get_expr_type(p->node->children->node->category); p = p->next; }

            if (is_method_redefined(global_table, id_node->token, p_types, p_count)) {
                
                /* Tabela dummy restaurada! Mas com limpeza à prova de bala! */
                struct symbol_table *dummy = create_table("dummy", "Method");
                struct node_list *p2 = params_node->children;
                while(p2) { 
                    check_and_insert(dummy, p2->node->children->next->node, get_expr_type(p2->node->children->node->category), 1);
                    p2 = p2->next; 
                }

                printf("Line %d, col %d: Symbol %s(", id_node->line, id_node->col, id_node->token);
                for(int i = 0; i < p_count; i++) {
                    printf("%s%s", get_type_name(p_types[i]), (i == p_count - 1) ? "" : ",");
                }
                printf(") already defined\n");
                semantic_errors++;
                decl->is_duplicate = 1; 
                free(p_types);
                
                /* LIMPEZA SEGURA: Sem Memory Leak (MLE) e Sem Segfault! */
                struct symbol *curr_sym = dummy->first_symbol;
                while (curr_sym) {
                    struct symbol *next_sym = curr_sym->next;
                    remove_from_cache(dummy, curr_sym); 
                    free(curr_sym->name);
                    if (curr_sym->param_types) free(curr_sym->param_types);
                    free(curr_sym);
                    curr_sym = next_sym;
                }
                free(dummy->table_name);
                free(dummy->table_type);
                free(dummy);

            } else {
                struct symbol *ms = insert_symbol(global_table, id_node->token, get_expr_type(header->children->node->category), 0, id_node->line, id_node->col);
                ms->is_method = 1;
                ms->num_params = p_count;
                ms->param_types = malloc(sizeof(ExprType) * (p_count > 0 ? p_count : 1));
                for(int i = 0; i < p_count; i++) ms->param_types[i] = p_types[i];

                char *sig = malloc(strlen(id_node->token) + p_count * 15 + 10);
                char *ptr = sig;
                ptr += sprintf(ptr, "%s(", id_node->token);
                for(int i = 0; i < p_count; i++) {
                    ptr += sprintf(ptr, "%s", get_type_name(p_types[i]));
                    if(i < p_count - 1) ptr += sprintf(ptr, ",");
                }
                sprintf(ptr, ")");
                
                struct symbol_table *mt = create_table(sig, "Method"); 
                add_table(mt);
                insert_symbol(mt, "return", get_expr_type(header->children->node->category), 0, 0, 0);
                
                p = params_node->children;
                while(p) { 
                    check_and_insert(mt, p->node->children->next->node, get_expr_type(p->node->children->node->category), 1);
                    p = p->next; 
                }
                free(sig);
                free(p_types);
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
        
        int p_count = 0;
        struct node_list *p = header->children->next->next->node->children;
        while(p) { p_count++; p = p->next; }
        
        char *sig = malloc(strlen(header->children->next->node->token) + p_count * 15 + 10);
        char *ptr = sig;
        ptr += sprintf(ptr, "%s(", header->children->next->node->token);
        p = header->children->next->next->node->children;
        while(p) {
            ptr += sprintf(ptr, "%s", get_type_name(get_expr_type(p->node->children->node->category)));
            if(p->next) ptr += sprintf(ptr, ",");
            p = p->next;
        }
        sprintf(ptr, ")");
        
        /* Pesquisa imediata! */
        method_table = get_table_from_cache(sig);
        free(sig);
    }
    
    struct node_list *child = node->children;
    
    if (node->category == If || node->category == While) {
        if (child && child->node) {
            annotate_ast(child->node, method_table);
            if (child->node->type != TypeBoolean) { 
                printf("Line %d, col %d: Incompatible type %s in %s statement\n", 
                       child->node->line, child->node->col, 
                       get_type_name(child->node->type), 
                       (node->category == If ? "if" : "while")); 
                semantic_errors++; 
            }
            child = child->next;
            while (child != NULL) {
                annotate_ast(child->node, method_table);
                child = child->next;
            }
        }
        return; 
    }
    
    while (child != NULL) {
        if (!(node->category == Call && child == node->children)) 
            annotate_ast(child->node, method_table);
        child = child->next;
    }
    
    switch (node->category) {
        case StrLit:
            node->type = TypeString;
            node->is_expr = 1;
            break;
            
        case VarDecl:
            if (method_table && node->children && node->children->next) {
                check_and_insert(method_table, node->children->next->node, get_expr_type(node->children->node->category), 0);
            }
            break;
            
        case Identifier:
            if (method_table && node->is_expr) {
                if (strcmp(node->token, "_") == 0) {
                    node->type = TypeUndef;
                    printf("Line %d, col %d: Symbol _ is reserved\n", node->line, node->col);
                    semantic_errors++;
                } else {
                    struct symbol *s = lookup_symbol(method_table, sym_tables, node->token, node->line, node->col);
                    if (s && !s->is_method) node->type = s->type;
                    else { node->type = TypeUndef; printf("Line %d, col %d: Cannot find symbol %s\n", node->line, node->col, node->token); semantic_errors++; }
                }
            }
            break;
            
        case Natural:
            node->type = TypeInt;
            {
                char *clean = malloc(strlen(node->token) + 1);
                strip_underscores(node->token, clean);
                if (atof(clean) > 2147483647.0) { printf("Line %d, col %d: Number %s out of bounds\n", node->line, node->col, node->token); semantic_errors++; }
                free(clean);
            }
            break;
            
        case Decimal:
            node->type = TypeDouble;
            {
                char *clean = malloc(strlen(node->token) + 1);
                strip_underscores(node->token, clean);
                double val = atof(clean);
                
                if (val == 0.0) {
                    int is_real_zero = 1;
                    for (int i = 0; clean[i]; i++) {
                        if (clean[i] == 'e' || clean[i] == 'E') break;
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
                else if (val * 0.0 != 0.0) {
                    printf("Line %d, col %d: Number %s out of bounds\n", node->line, node->col, node->token);
                    semantic_errors++;
                }
                free(clean);
            }
            break;
            
        case BoolLit: 
            node->type = TypeBoolean; 
            break;
        
        case Add: case Sub: case Mul: case Div: case Mod:
            node->is_expr = 1;
            if (node->children && node->children->next) {
                ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
                if ((t1 == TypeInt || t1 == TypeDouble) && (t2 == TypeInt || t2 == TypeDouble)) {
                    node->type = (t1 == TypeDouble || t2 == TypeDouble) ? TypeDouble : TypeInt;
                } else {
                    node->type = TypeUndef;
                    char op = (node->category == Add) ? '+' : (node->category == Sub) ? '-' : (node->category == Mul) ? '*' : (node->category == Div) ? '/' : '%';
                    printf("Line %d, col %d: Operator %c cannot be applied to types %s, %s\n", node->line, node->col, op, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                }
            } else { node->type = TypeUndef; }
            break;
            
        case Assign:
            node->is_expr = 1;
            if (node->children && node->children->next) {
                ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
                node->type = t1;
                int is_valid = ((t1 == t2) && t1 != TypeStringArray && t1 != TypeUndef && t1 != TypeNone) || (t1 == TypeDouble && t2 == TypeInt);
                
                if (!is_valid) {
                    printf("Line %d, col %d: Operator = cannot be applied to types %s, %s\n", node->line, node->col, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                }
            } else { node->type = TypeUndef; }
            break;
            
        case Call:
            node->is_expr = 1;
            if (node->children && node->children->node) {
                struct node *id_node = node->children->node; 
                id_node->is_expr = 1;
                struct node_list *arg = node->children->next; 
                int count = 0; 
                while(arg) { count++; arg = arg->next; }
                
                ExprType *at = malloc(sizeof(ExprType) * (count > 0 ? count : 1));
                char *call_sig = malloc(strlen(id_node->token) + count * 15 + 10);
                char *ptr = call_sig;
                
                ptr += sprintf(ptr, "%s(", id_node->token);
                arg = node->children->next;
                int idx = 0;
                while(arg) { 
                    at[idx] = arg->node->type; 
                    ptr += sprintf(ptr, "%s", get_type_name(at[idx])); 
                    if(arg->next) ptr += sprintf(ptr, ","); 
                    arg = arg->next; 
                    idx++; 
                }
                sprintf(ptr, ")");

                if (strcmp(id_node->token, "_") == 0) {
                    node->type = TypeUndef; 
                    id_node->func_sig = strdup("undef");
                    printf("Line %d, col %d: Symbol _ is reserved\n", id_node->line, id_node->col); 
                    semantic_errors++;
                    free(at);
                    free(call_sig);
                    break;
                }
                
                struct symbol *best = NULL; 
                unsigned long h = hash_str(id_node->token);
                struct symbol_cache_node *c_node = sym_cache[h];
                
                while(c_node) {
                    if (c_node->table == sym_tables && c_node->sym->is_method && strcmp(c_node->sym->name, id_node->token) == 0 && c_node->sym->num_params == count) {
                        int match = 1; 
                        for(int i = 0; i < count; i++) {
                            if(c_node->sym->param_types[i] != at[i]) { match = 0; break; }
                        }
                        if(match) { best = c_node->sym; break; }
                    }
                    c_node = c_node->next;
                }
                
                int match_count = 0;
                struct symbol *ambiguous_best = NULL;

                if (!best) {
                    c_node = sym_cache[h];
                    while(c_node) {
                        if (c_node->table == sym_tables && c_node->sym->is_method && strcmp(c_node->sym->name, id_node->token) == 0 && c_node->sym->num_params == count) {
                            int match = 1; 
                            for(int i = 0; i < count; i++) {
                                ExprType param = c_node->sym->param_types[i];
                                if (!(param == at[i] || (param == TypeDouble && at[i] == TypeInt))) {
                                    match = 0; break;
                                }
                            }
                            if(match) { ambiguous_best = c_node->sym; match_count++; }
                        }
                        c_node = c_node->next;
                    }
                    if (match_count == 1) best = ambiguous_best;
                }
                
                if (best) {
                    node->type = best->type; 
                    char *rs = malloc(best->num_params * 15 + 10);
                    char *r_ptr = rs;
                    r_ptr += sprintf(r_ptr, "(");
                    for(int i = 0; i < best->num_params; i++) { 
                        r_ptr += sprintf(r_ptr, "%s", get_type_name(best->param_types[i])); 
                        if(i < best->num_params - 1) r_ptr += sprintf(r_ptr, ","); 
                    }
                    sprintf(r_ptr, ")"); 
                    id_node->func_sig = strdup(rs);
                    free(rs);
                } else if (match_count > 1) {
                    node->type = TypeUndef; 
                    id_node->func_sig = strdup("undef");
                    printf("Line %d, col %d: Reference to method %s is ambiguous\n", id_node->line, id_node->col, call_sig); 
                    semantic_errors++;
                } else {
                    node->type = TypeUndef; 
                    id_node->func_sig = strdup("undef");
                    printf("Line %d, col %d: Cannot find symbol %s\n", id_node->line, id_node->col, call_sig); 
                    semantic_errors++;
                }
                free(at);
                free(call_sig);
            } else { node->type = TypeUndef; }
            break;
            
        case Return:
            if (method_table) {
                struct symbol *ret = lookup_symbol(method_table, NULL, "return", 0, 0);
                ExprType exp = ret ? ret->type : TypeVoid;
                if (node->children == NULL) { 
                    if (exp != TypeVoid) { printf("Line %d, col %d: Incompatible type void in return statement\n", node->line, node->col); semantic_errors++; } 
                }
                else if (node->children->node) { 
                    ExprType act = node->children->node->type;
                    if (exp == TypeVoid || (exp != act && !(exp == TypeDouble && act == TypeInt))) { printf("Line %d, col %d: Incompatible type %s in return statement\n", node->children->node->line, node->children->node->col, get_type_name(act)); semantic_errors++; }
                }
            }
            break;
            
        case Print:
            if (node->children && node->children->node) {
                ExprType t1 = node->children->node->type;
                if (t1 != TypeInt && t1 != TypeDouble && t1 != TypeBoolean && t1 != TypeString) {
                    printf("Line %d, col %d: Incompatible type %s in System.out.print statement\n", node->children->node->line, node->children->node->col, get_type_name(t1));
                    semantic_errors++;
                }
            }
            break;
            
        case ParseArgs:
            node->type = TypeInt; node->is_expr = 1;
            if (node->children && node->children->next && node->children->node && node->children->next->node) {
                if (node->children->node->type != TypeStringArray || node->children->next->node->type != TypeInt) {
                    printf("Line %d, col %d: Operator Integer.parseInt cannot be applied to types %s, %s\n", node->line, node->col, get_type_name(node->children->node->type), get_type_name(node->children->next->node->type));
                    semantic_errors++;
                }
            }
            break;
            
        case Length:
            node->type = TypeInt; node->is_expr = 1;
            if (node->children && node->children->node) {
                if (node->children->node->type != TypeStringArray) { printf("Line %d, col %d: Operator .length cannot be applied to type %s\n", node->line, node->col, get_type_name(node->children->node->type)); semantic_errors++; }
            }
            break;
            
        case And: case Or:
            node->is_expr = 1; node->type = TypeBoolean;
            if (node->children && node->children->next) {
              ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              if (t1 != TypeBoolean || t2 != TypeBoolean) { printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, (node->category == And ? "&&" : "||"), get_type_name(t1), get_type_name(t2)); semantic_errors++; }
            }
            break;
            
        case Xor:
            node->is_expr = 1;
            if (node->children && node->children->next) {
                ExprType t1 = node->children->node->type;
                ExprType t2 = node->children->next->node->type;
                
                if (t1 == TypeInt && t2 == TypeInt) {
                    node->type = TypeInt;
                } else {
                    node->type = TypeInt; 
                    printf("Line %d, col %d: Operator ^ cannot be applied to types %s, %s\n", node->line, node->col, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                }
            } else { node->type = TypeUndef; }
            break;
            
        case Lshift: case Rshift:
            node->is_expr = 1; node->type = TypeInt;
            if (node->children && node->children->next) {
              ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              if (t1 != TypeInt || t2 != TypeInt) { printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, (node->category == Lshift ? "<<" : ">>"), get_type_name(t1), get_type_name(t2)); semantic_errors++; }
            }
            break;
            
        case Not:
            node->is_expr = 1; node->type = TypeBoolean;
            if (node->children && node->children->node) {
                if (node->children->node->type != TypeBoolean) { printf("Line %d, col %d: Operator ! cannot be applied to type %s\n", node->line, node->col, get_type_name(node->children->node->type)); semantic_errors++; }
            }
            break;
            
        case Eq: case Ne:
            node->is_expr = 1; node->type = TypeBoolean;
            if (node->children && node->children->next) {
              ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              int v = ((t1 == TypeBoolean && t2 == TypeBoolean) || ((t1 == TypeInt || t1 == TypeDouble) && (t2 == TypeInt || t2 == TypeDouble)));
              if (!v) { printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, (node->category == Eq ? "==" : "!="), get_type_name(t1), get_type_name(t2)); semantic_errors++; }
            }
            break;
            
        case Lt: case Gt: case Le: case Ge:
            node->is_expr = 1; node->type = TypeBoolean;
            if (node->children && node->children->next) {
              ExprType t1 = node->children->node->type, t2 = node->children->next->node->type;
              if (!((t1 == TypeInt || t1 == TypeDouble) && (t2 == TypeInt || t2 == TypeDouble))) {
                  char *op = "<"; if(node->category == Gt) op = ">"; else if(node->category == Le) op = "<="; else if(node->category == Ge) op = ">=";
                  printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n", node->line, node->col, op, get_type_name(t1), get_type_name(t2)); semantic_errors++;
              }
            }
            break;
            
        case Plus: case Minus:
            node->is_expr = 1;
            if (node->children && node->children->node) {
              ExprType t1 = node->children->node->type;
              if (t1 == TypeInt || t1 == TypeDouble) node->type = t1;
              else { node->type = TypeUndef; printf("Line %d, col %d: Operator %s cannot be applied to type %s\n", node->line, node->col, (node->category == Plus ? "+" : "-"), get_type_name(t1)); semantic_errors++; }
            } else { node->type = TypeUndef; }
            break;
            
        default: break;
    }
}