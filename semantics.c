#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"

struct symbol_table *sym_tables = NULL;
int semantic_errors = 0;

/* Remove underscores de strings numericas (ex: 1_000 -> 1000) */
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
    if (sym_tables == NULL) {
        sym_tables = new_table;
        return;
    }
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
        semantic_errors++;
        return;
    }
    struct symbol *curr = table->first_symbol;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            printf("Line %d, col %d: Symbol %s already defined\n", id_node->line, id_node->col, name);
            semantic_errors++;
            return;
        }
        curr = curr->next;
    }
    insert_symbol(table, name, type, is_param, id_node->line, id_node->col);
}

struct symbol *lookup_symbol(struct symbol_table *local_table, struct symbol_table *global_table, char *name, int use_line, int use_col) {
    struct symbol *sym = local_table ? local_table->first_symbol : NULL;
    while (sym != NULL) {
        if (strcmp(sym->name, name) == 0) {
            if (sym->is_param || sym->line < use_line || (sym->line == use_line && sym->col < use_col) || sym->line == 0) return sym;
        }
        sym = sym->next;
    }
    sym = global_table ? global_table->first_symbol : NULL;
    while (sym != NULL) {
        if (strcmp(sym->name, name) == 0) return sym;
        sym = sym->next;
    }
    return NULL;
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
            ExprType field_type = get_expr_type(decl->children->node->category);
            check_and_insert(global_table, decl->children->next->node, field_type, 0); 
        }
        else if (decl->category == MethodDecl) {
            struct node *header = decl->children->node;
            ExprType return_type = get_expr_type(header->children->node->category);
            struct node *id_node = header->children->next->node;
            struct node *params_node = header->children->next->next->node;
            struct symbol *ms = insert_symbol(global_table, id_node->token, return_type, 0, id_node->line, id_node->col);
            ms->is_method = 1;
            char sig[512]; sprintf(sig, "%s(", id_node->token);
            struct node_list *p = params_node->children;
            while(p) {
                ExprType pt = get_expr_type(p->node->children->node->category);
                ms->param_types = realloc(ms->param_types, sizeof(ExprType)*(ms->num_params+1));
                ms->param_types[ms->num_params++] = pt;
                strcat(sig, get_type_name(pt)); if(p->next) strcat(sig, ",");
                p = p->next;
            }
            strcat(sig, ")");
            struct symbol_table *mt = create_table(sig, "Method"); add_table(mt);
            insert_symbol(mt, "return", return_type, 0, 0, 0);
            p = params_node->children;
            while(p) { 
                check_and_insert(mt, p->node->children->next->node, get_expr_type(p->node->children->node->category), 1);
                p = p->next; 
            }
            struct node *body = decl->children->next->node;
            if (body) {
                struct node_list *bc = body->children;
                while(bc) {
                    if (bc->node->category == VarDecl)
                        check_and_insert(mt, bc->node->children->next->node, get_expr_type(bc->node->children->node->category), 0);
                    bc = bc->next;
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
    if (node == NULL) return;
    if (node->category == MethodDecl) {
        struct node *header = node->children->node;
        char *method_name = header->children->next->node->token;
        struct node *params_node = header->children->next->next->node;
        char sig[512]; sprintf(sig, "%s(", method_name);
        struct node_list *p = params_node->children;
        while(p) {
            strcat(sig, get_type_name(get_expr_type(p->node->children->node->category)));
            if(p->next) strcat(sig, ",");
            p = p->next;
        }
        strcat(sig, ")");
        struct symbol_table *curr = sym_tables;
        while (curr) {
            if (strcmp(curr->table_name, sig) == 0) { method_table = curr; break; }
            curr = curr->next;
        }
    }
    struct node_list *child = node->children;
    while (child != NULL) {
        if (!(node->category == Call && child == node->children)) annotate_ast(child->node, method_table);
        child = child->next;
    }
    switch (node->category) {
        case Identifier:
            if (method_table && node->is_expr) {
                struct symbol *s = lookup_symbol(method_table, sym_tables, node->token, node->line, node->col);
                if (s && !s->is_method) node->type = s->type;
                else { 
                    node->type = TypeUndef; 
                    printf("Line %d, col %d: Cannot find symbol %s\n", node->line, node->col, node->token);
                    semantic_errors++;
                }
            }
            break;
        case Natural:
            node->type = TypeInt;
            {
                char clean[256]; strip_underscores(node->token, clean);
                if (atof(clean) > 2147483647.0) {
                    printf("Line %d, col %d: Number %s out of bounds\n", node->line, node->col, node->token);
                    semantic_errors++;
                }
            }
            break;
        case Decimal: node->type = TypeDouble; break;
        case BoolLit: node->type = TypeBoolean; break;
        case Add: case Sub: case Mul: case Div: case Mod:
            node->is_expr = 1;
            {
                ExprType t1 = node->children->node->type;
                ExprType t2 = node->children->next->node->type;
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
                if (t1 != t2 && !(t1 == TypeDouble && t2 == TypeInt)) {
                    printf("Line %d, col %d: Operator = cannot be applied to types %s, %s\n", node->line, node->col, get_type_name(t1), get_type_name(t2));
                    semantic_errors++;
                }
            }
            break;
        case Call:
            node->is_expr = 1;
            {
                struct node *id_node = node->children->node; id_node->is_expr = 1;
                struct symbol *func = lookup_symbol(NULL, sym_tables, id_node->token, 0, 0);
                char call_sig[512]; sprintf(call_sig, "%s(", id_node->token);
                struct node_list *arg = node->children->next; int count = 0;
                while(arg) {
                    strcat(call_sig, get_type_name(arg->node->type));
                    if(arg->next) strcat(call_sig, ",");
                    arg = arg->next; count++;
                }
                strcat(call_sig, ")");
                if (func && func->is_method && func->num_params == count) {
                    node->type = func->type;
                    char rs[512] = "(";
                    for(int i=0; i<func->num_params; i++) {
                        strcat(rs, get_type_name(func->param_types[i]));
                        if(i < func->num_params-1) strcat(rs, ",");
                    }
                    strcat(rs, ")"); id_node->func_sig = strdup(rs);
                } else {
                    node->type = TypeUndef; id_node->func_sig = strdup("undef");
                    printf("Line %d, col %d: Cannot find symbol %s\n", id_node->line, id_node->col, call_sig);
                    semantic_errors++;
                }
            }
            break;
        case Return:
            if (method_table) {
                struct symbol *ret = lookup_symbol(method_table, NULL, "return", 0, 0);
                ExprType exp = ret ? ret->type : TypeVoid;
                if (node->children == NULL) {
                    if (exp != TypeVoid) {
                        printf("Line %d, col %d: Incompatible type void in return statement\n", node->line, node->col);
                        semantic_errors++;
                    }
                } else {
                    ExprType act = node->children->node->type;
                    if (exp == TypeVoid || (exp != act && !(exp == TypeDouble && act == TypeInt))) {
                        printf("Line %d, col %d: Incompatible type %s in return statement\n", node->children->node->line, node->children->node->col, get_type_name(act));
                        semantic_errors++;
                    }
                }
            }
            break;
        case Print:
            {
                ExprType t = node->children->node->type;
                if (node->children->node->category != StrLit && (t == TypeVoid || t == TypeStringArray || t == TypeUndef)) {
                    printf("Line %d, col %d: Incompatible type %s in System.out.print statement\n", node->children->node->line, node->children->node->col, get_type_name(t));
                    semantic_errors++;
                }
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
            if (node->children->node->type != TypeStringArray) {
                printf("Line %d, col %d: Operator .length cannot be applied to type %s\n", node->line, node->col, get_type_name(node->children->node->type));
                semantic_errors++;
            }
            break;
        default: break;
    }
}