#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>

#define FK_NAME_MAX 128
#define FK_INITIAL_CAP 16
#define FK_MAX_LINE 8192

typedef enum { V_NONE, V_NUM, V_STR, V_BOOL } ValueType;

typedef struct {
    ValueType type;
    long long num;
    int boolean;
    char *str;
} Value;

typedef struct {
    char name[FK_NAME_MAX];
    long long start;              /* user-visible first index */
    long long size;               /* highest allocated index count from start */
    long long cap;
    Value *items;
} Sell;

typedef struct {
    Sell *sells;
    size_t count;
    size_t cap;
} Env;

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
    Env *env;
} Expr;

typedef struct {
    const char *p;
    size_t len;
    size_t pos;
} Source;

static void fk_error(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "FK error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) fk_error("out of memory");
    return p;
}

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) fk_error("out of memory");
    return q;
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)xmalloc(n);
    memcpy(p, s, n);
    return p;
}

static Value v_none(void) { Value v = { V_NONE, 0, 0, NULL }; return v; }
static Value v_num(long long n) { Value v = { V_NUM, n, 0, NULL }; return v; }
static Value v_bool(int b) { Value v = { V_BOOL, 0, b ? 1 : 0, NULL }; return v; }
static Value v_str(const char *s) { Value v = { V_STR, 0, 0, xstrdup(s) }; return v; }

static void v_free(Value *v) {
    if (v->type == V_STR) free(v->str);
    *v = v_none();
}

static Value v_copy(const Value *v) {
    if (v->type == V_STR) return v_str(v->str ? v->str : "");
    return *v;
}

static int truthy(const Value *v) {
    switch (v->type) {
        case V_BOOL: return v->boolean != 0;
        case V_NUM: return v->num != 0;
        case V_STR: return v->str && v->str[0] != '\0';
        default: return 0;
    }
}

static long long as_num(const Value *v) {
    if (v->type == V_NUM) return v->num;
    if (v->type == V_BOOL) return v->boolean ? 1 : 0;
    if (v->type == V_STR) {
        char *e = NULL;
        long long n = strtoll(v->str, &e, 10);
        if (e && *e == '\0') return n;
    }
    fk_error("numeric value expected");
    return 0;
}

static int value_eq(const Value *a, const Value *b) {
    if (a->type == V_STR || b->type == V_STR) {
        if (a->type != V_STR || b->type != V_STR) return 0;
        return strcmp(a->str ? a->str : "", b->str ? b->str : "") == 0;
    }
    return as_num(a) == as_num(b);
}

static void print_value(const Value *v) {
    switch (v->type) {
        case V_NUM: printf("%lld", v->num); break;
        case V_BOOL: fputs(v->boolean ? "true" : "false", stdout); break;
        case V_STR: fputs(v->str ? v->str : "", stdout); break;
        default: fputs("none", stdout); break;
    }
}

static Sell *find_sell(Env *e, const char *name) {
    for (size_t i = 0; i < e->count; ++i)
        if (strcmp(e->sells[i].name, name) == 0) return &e->sells[i];
    return NULL;
}

static Sell *get_sell(Env *e, const char *name) {
    Sell *s = find_sell(e, name);
    if (s) return s;
    if (e->count == e->cap) {
        e->cap = e->cap ? e->cap * 2 : 8;
        e->sells = (Sell *)xrealloc(e->sells, e->cap * sizeof(Sell));
    }
    s = &e->sells[e->count++];
    memset(s, 0, sizeof(*s));
    snprintf(s->name, sizeof(s->name), "%s", name);
    s->start = 1;
    return s;
}

static void sell_init(Sell *s, long long start, long long end) {
    if (end < start) fk_error("sell range must be start~end");
    s->start = start;
    s->size = end - start + 1;
    if (s->size < 0) fk_error("sell range overflow");
    long long need = s->size;
    long long cap = FK_INITIAL_CAP;
    while (cap < need) {
        if (cap > LLONG_MAX / 2) { cap = need; break; }
        cap *= 2;
    }
    if (cap > 0) {
        s->items = (Value *)xmalloc((size_t)cap * sizeof(Value));
        for (long long i = 0; i < cap; ++i) s->items[i] = v_none();
    }
    s->cap = cap;
}

static void sell_ensure_index(Sell *s, long long idx) {
    if (idx < 0) fk_error("negative sell index: %lld", idx);
    if (s->size == 0) {
        s->start = 1;
        if (idx >= s->start) s->size = idx - s->start + 1;
        else fk_error("index %lld is below sell start", idx);
    } else {
        if (idx < s->start) fk_error("index %lld is below sell start %lld", idx, s->start);
        if (idx < s->start + s->size) return;
        s->size = idx - s->start + 1;
    }
    if (s->size > s->cap) {
        long long cap = s->cap ? s->cap : FK_INITIAL_CAP;
        while (cap < s->size) {
            if (cap > LLONG_MAX / 2) { cap = s->size; break; }
            cap *= 2;
        }
        long long old = s->cap;
        s->items = (Value *)xrealloc(s->items, (size_t)cap * sizeof(Value));
        for (long long i = old; i < cap; ++i) s->items[i] = v_none();
        s->cap = cap;
    }
}

static Value sell_get(Sell *s, long long idx) {
    if (!s) return v_none();
    if (idx < s->start || idx >= s->start + s->size)
        fk_error("sell '%s' index %lld is not initialized", s->name, idx);
    return v_copy(&s->items[idx - s->start]);
}

static void sell_set(Sell *s, long long idx, Value v) {
    sell_ensure_index(s, idx);
    v_free(&s->items[idx - s->start]);
    s->items[idx - s->start] = v;
}

static void skip_ws(Expr *p) {
    while (p->pos < p->len && isspace((unsigned char)p->src[p->pos])) p->pos++;
}

static int eat(Expr *p, const char *token) {
    skip_ws(p);
    size_t n = strlen(token);
    if (p->pos + n <= p->len && strncmp(p->src + p->pos, token, n) == 0) {
        p->pos += n;
        return 1;
    }
    return 0;
}

static void expect_char(Expr *p, char c) {
    skip_ws(p);
    if (p->pos >= p->len || p->src[p->pos] != c)
        fk_error("expected '%c' in expression", c);
    p->pos++;
}

static Value parse_expr(Expr *p);

static Value parse_string(Expr *p) {
    expect_char(p, '"');
    char *buf = (char *)xmalloc(64);
    size_t cap = 64, n = 0;
    while (p->pos < p->len) {
        char c = p->src[p->pos++];
        if (c == '"') {
            buf[n] = '\0';
            Value v = v_str(buf);
            free(buf);
            return v;
        }
        if (c == '\\' && p->pos < p->len) {
            char e = p->src[p->pos++];
            if (e == 'n') c = '\n';
            else if (e == 't') c = '\t';
            else c = e;
        }
        if (n + 2 > cap) {
            cap *= 2;
            buf = (char *)xrealloc(buf, cap);
        }
        buf[n++] = c;
    }
    free(buf);
    fk_error("unterminated string");
    return v_none();
}

static void parse_identifier(Expr *p, char *out, size_t out_cap) {
    skip_ws(p);
    size_t n = 0;
    while (p->pos < p->len) {
        char c = p->src[p->pos];
        if (!(isalnum((unsigned char)c) || c == '_')) break;
        if (n + 1 < out_cap) out[n++] = c;
        p->pos++;
    }
    if (n == 0) fk_error("identifier expected");
    out[n] = '\0';
}

static Value parse_primary(Expr *p) {
    skip_ws(p);
    if (p->pos >= p->len) fk_error("unexpected end of expression");

    if (p->src[p->pos] == '"') return parse_string(p);
    if (eat(p, "(")) {
        Value v = parse_expr(p);
        expect_char(p, ')');
        return v;
    }

    if (isdigit((unsigned char)p->src[p->pos])) {
        char *e = NULL;
        long long n = strtoll(p->src + p->pos, &e, 10);
        if (e == p->src + p->pos) fk_error("invalid number");
        p->pos = (size_t)(e - p->src);
        return v_num(n);
    }

    char id[FK_NAME_MAX];
    parse_identifier(p, id, sizeof(id));

    if (strcmp(id, "true") == 0 || strcmp(id, "white") == 0 || strcmp(id, "yes") == 0)
        return v_bool(1);
    if (strcmp(id, "false") == 0 || strcmp(id, "black") == 0 || strcmp(id, "no") == 0)
        return v_bool(0);

    /* FK expression form: holy = sell index(1) */
    skip_ws(p);
    size_t save = p->pos;
    if (eat(p, "=")) {
        if (!eat(p, "sell")) fk_error("only 'name = sell index(...)' is valid inside an expression");
        if (!eat(p, "index")) fk_error("expected 'index' after 'sell'");
        expect_char(p, '(');
        Value ix = parse_expr(p);
        expect_char(p, ')');
        long long idx = as_num(&ix);
        v_free(&ix);
        Sell *s = find_sell(p->env, id);
        if (!s) fk_error("unknown sell '%s'", id);
        return sell_get(s, idx);
    }
    p->pos = save;

    fk_error("unknown value '%s'", id);
    return v_none();
}

static Value parse_unary(Expr *p) {
    if (eat(p, "!")) {
        Value v = parse_unary(p);
        int r = !truthy(&v);
        v_free(&v);
        return v_bool(r);
    }
    if (eat(p, "-")) {
        Value v = parse_unary(p);
        long long n = -as_num(&v);
        v_free(&v);
        return v_num(n);
    }
    return parse_primary(p);
}

static Value add_values(Value a, Value b) {
    if (a.type == V_STR || b.type == V_STR) {
        char left[FK_MAX_LINE], right[FK_MAX_LINE];
        if (a.type == V_STR) snprintf(left, sizeof(left), "%s", a.str ? a.str : "");
        else if (a.type == V_NUM) snprintf(left, sizeof(left), "%lld", a.num);
        else snprintf(left, sizeof(left), "%s", truthy(&a) ? "true" : "false");
        if (b.type == V_STR) snprintf(right, sizeof(right), "%s", b.str ? b.str : "");
        else if (b.type == V_NUM) snprintf(right, sizeof(right), "%lld", b.num);
        else snprintf(right, sizeof(right), "%s", truthy(&b) ? "true" : "false");
        size_t n = strlen(left) + strlen(right) + 1;
        char *out = (char *)xmalloc(n);
        snprintf(out, n, "%s%s", left, right);
        Value v = v_str(out);
        free(out);
        v_free(&a); v_free(&b);
        return v;
    }
    long long n = as_num(&a) + as_num(&b);
    v_free(&a); v_free(&b);
    return v_num(n);
}

static Value parse_mul(Expr *p) {
    Value a = parse_unary(p);
    for (;;) {
        if (eat(p, "*")) {
            Value b = parse_unary(p); long long n = as_num(&a) * as_num(&b);
            v_free(&a); v_free(&b); a = v_num(n);
        } else if (eat(p, "/")) {
            Value b = parse_unary(p); long long d = as_num(&b);
            if (d == 0) fk_error("division by zero");
            long long n = as_num(&a) / d;
            v_free(&a); v_free(&b); a = v_num(n);
        } else if (eat(p, "%")) {
            Value b = parse_unary(p); long long d = as_num(&b);
            if (d == 0) fk_error("modulo by zero");
            long long n = as_num(&a) % d;
            v_free(&a); v_free(&b); a = v_num(n);
        } else break;
    }
    return a;
}

static Value parse_add(Expr *p) {
    Value a = parse_mul(p);
    for (;;) {
        if (eat(p, "+")) {
            Value b = parse_mul(p); a = add_values(a, b);
        } else if (eat(p, "-")) {
            Value b = parse_mul(p); long long n = as_num(&a) - as_num(&b);
            v_free(&a); v_free(&b); a = v_num(n);
        } else break;
    }
    return a;
}

static Value parse_cmp(Expr *p) {
    Value a = parse_add(p);
    for (;;) {
        int op = 0;
        if (eat(p, "==")) op = 1;
        else if (eat(p, "!=")) op = 2;
        else if (eat(p, ">=")) op = 3;
        else if (eat(p, "<=")) op = 4;
        else if (eat(p, ">")) op = 5;
        else if (eat(p, "<")) op = 6;
        else break;
        Value b = parse_add(p); int r = 0;
        if (op == 1) r = value_eq(&a, &b);
        else if (op == 2) r = !value_eq(&a, &b);
        else { long long x = as_num(&a), y = as_num(&b);
            if (op == 3) r = x >= y; else if (op == 4) r = x <= y;
            else if (op == 5) r = x > y; else r = x < y;
        }
        v_free(&a); v_free(&b); a = v_bool(r);
    }
    return a;
}

static Value parse_and(Expr *p) {
    Value a = parse_cmp(p);
    while (eat(p, "&&")) {
        Value b = parse_cmp(p); int r = truthy(&a) && truthy(&b);
        v_free(&a); v_free(&b); a = v_bool(r);
    }
    return a;
}

static Value parse_expr(Expr *p) {
    Value a = parse_and(p);
    while (eat(p, "||")) {
        Value b = parse_and(p); int r = truthy(&a) || truthy(&b);
        v_free(&a); v_free(&b); a = v_bool(r);
    }
    return a;
}

static void check_expr_done(Expr *p) {
    skip_ws(p);
    if (p->pos != p->len) fk_error("unexpected expression text near: %s", p->src + p->pos);
}

static int line_is_comment(const char *line) {
    while (*line && isspace((unsigned char)*line)) line++;
    return strncmp(line, "//;;##", 6) == 0;
}

static void execute_source(Env *env, const char *src);

static const char *skip_string(const char *p, const char *end) {
    p++;
    while (p < end) {
        if (*p == '\\' && p + 1 < end) { p += 2; continue; }
        if (*p == '"') return p + 1;
        p++;
    }
    fk_error("unterminated string");
    return end;
}

static const char *find_matching(const char *open, const char *end, char left, char right) {
    int depth = 0;
    for (const char *p = open; p < end; ++p) {
        if (*p == '"') { p = skip_string(p, end) - 1; continue; }
        if (*p == left) depth++;
        else if (*p == right) {
            if (--depth == 0) return p;
        }
    }
    fk_error("unclosed '%c' block", left);
    return end;
}

static void trim_span(const char **a, const char **b) {
    while (*a < *b && isspace((unsigned char)**a)) (*a)++;
    while (*b > *a && isspace((unsigned char)*((*b) - 1))) (*b)--;
}

static void execute_statement_span(Env *env, const char *a, const char *b) {
    trim_span(&a, &b);
    if (a >= b) return;

    size_t n = (size_t)(b - a);
    char *stmt = (char *)xmalloc(n + 1);
    memcpy(stmt, a, n); stmt[n] = '\0';
    if (line_is_comment(stmt)) { free(stmt); return; }

    if (stmt[0] == '[') {
        const char *close = find_matching(stmt, stmt + n, '[', ']');
        const char *q = close + 1;
        while (q < stmt + n && isspace((unsigned char)*q)) q++;
        if (q >= stmt + n || *q != '(') fk_error("repeat needs '(count)'");
        const char *rp = find_matching(q, stmt + n, '(', ')');
        char *cnts = (char *)xmalloc((size_t)(rp - q));
        memcpy(cnts, q + 1, (size_t)(rp - q - 1)); cnts[rp - q - 1] = '\0';
        Expr ep = { cnts, 0, strlen(cnts), env };
        Value cv = parse_expr(&ep); check_expr_done(&ep);
        long long count = as_num(&cv); v_free(&cv); free(cnts);
        if (count < 0) fk_error("repeat count cannot be negative");
        {
            size_t body_len = (size_t)(close - (stmt + 1));
            char *body = (char *)xmalloc(body_len + 1);
            memcpy(body, stmt + 1, body_len); body[body_len] = '\0';
            for (long long i = 0; i < count; ++i) execute_source(env, body);
            free(body);
        }
        free(stmt); return;
    }

    if (strncmp(stmt, "if", 2) == 0 && (isspace((unsigned char)stmt[2]) || stmt[2] == '{')) {
        const char *lb = strchr(stmt, '{');
        if (!lb) fk_error("if needs '{'");
        const char *rb = find_matching(lb, stmt + n, '{', '}');
        const char *ca = stmt + 2, *cb = lb;
        trim_span(&ca, &cb);
        if (ca >= cb) fk_error("if needs a condition");
        char *cond = (char *)xmalloc((size_t)(cb - ca) + 1);
        memcpy(cond, ca, (size_t)(cb - ca)); cond[cb - ca] = '\0';
        Expr ep = { cond, 0, strlen(cond), env };
        Value v = parse_expr(&ep); check_expr_done(&ep);
        int yes = truthy(&v); v_free(&v); free(cond);
        if (yes) {
            size_t body_len = (size_t)(rb - (lb + 1));
            char *body = (char *)xmalloc(body_len + 1);
            memcpy(body, lb + 1, body_len); body[body_len] = '\0';
            execute_source(env, body);
            free(body);
        }
        if (rb + 1 < stmt + n) {
            const char *rest = rb + 1; while (rest < stmt + n && isspace((unsigned char)*rest)) rest++;
            if (*rest) fk_error("unexpected text after if block");
        }
        free(stmt); return;
    }

    if (strncmp(stmt, "prinf", 5) == 0) {
        const char *q = stmt + 5; while (isspace((unsigned char)*q)) q++;
        if (*q != '(') fk_error("prinf needs '(...)'");
        const char *rp = find_matching(q, stmt + n, '(', ')');
        const char *end = rp + 1; while (end < stmt + n && isspace((unsigned char)*end)) end++;
        if (end != stmt + n) fk_error("unexpected text after prinf()");
        size_t elen = (size_t)(rp - q - 1);
        char *expr = (char *)xmalloc(elen + 1); memcpy(expr, q + 1, elen); expr[elen] = '\0';
        Expr ep = { expr, 0, elen, env };
        Value v = parse_expr(&ep); check_expr_done(&ep);
        print_value(&v); putchar('\n'); v_free(&v); free(expr); free(stmt); return;
    }

    const char *eq = strchr(stmt, '=');
    if (!eq) fk_error("unknown statement: %s", stmt);
    const char *na = stmt, *nb = eq;
    trim_span(&na, &nb);
    if (na >= nb || (size_t)(nb - na) >= FK_NAME_MAX) fk_error("invalid sell name");
    char name[FK_NAME_MAX]; memcpy(name, na, (size_t)(nb - na)); name[nb - na] = '\0';
    for (char *c = name; *c; ++c) if (!(isalnum((unsigned char)*c) || *c == '_')) fk_error("invalid sell name '%s'", name);

    const char *rhs = eq + 1; while (*rhs && isspace((unsigned char)*rhs)) rhs++;
    if (strncmp(rhs, "sell", 4) == 0 && (rhs[4] == '\0' || isspace((unsigned char)rhs[4]))) {
        rhs += 4; while (*rhs && isspace((unsigned char)*rhs)) rhs++;
        if (strncmp(rhs, "make", 4) == 0 && (rhs[4] == '\0' || isspace((unsigned char)rhs[4]))) {
            rhs += 4; while (*rhs && isspace((unsigned char)*rhs)) rhs++;
            Sell *s = get_sell(env, name);
            if (!*rhs) {
                free(s->items); s->items = NULL; s->cap = 0; s->size = 0; s->start = 1;
            } else {
                char *e1 = NULL; long long start = strtoll(rhs, &e1, 10);
                if (e1 == rhs) fk_error("invalid sell start");
                while (isspace((unsigned char)*e1)) e1++;
                if (*e1 != '~') fk_error("sell range must use '~'");
                char *e2 = NULL; long long end = strtoll(e1 + 1, &e2, 10);
                if (e2 == e1 + 1) fk_error("invalid sell end");
                while (isspace((unsigned char)*e2)) e2++;
                if (*e2) fk_error("unexpected text after sell range");
                free(s->items); s->items = NULL; s->cap = 0; s->size = 0;
                sell_init(s, start, end);
            }
            free(stmt); return;
        }
    }

    /* name = index(n) > expr */
    if (strncmp(rhs, "index", 5) == 0 && (rhs[5] == '(' || isspace((unsigned char)rhs[5]))) {
        rhs += 5; while (*rhs && isspace((unsigned char)*rhs)) rhs++;
        if (*rhs != '(') fk_error("index assignment needs '(...)'");
        const char *rp = rhs;
        int dep = 0; for (; *rp; ++rp) { if (*rp == '(') dep++; else if (*rp == ')' && --dep == 0) break; }
        if (!*rp) fk_error("unclosed index(...) in assignment");
        char *ixs = (char *)xmalloc((size_t)(rp - rhs));
        memcpy(ixs, rhs + 1, (size_t)(rp - rhs - 1)); ixs[rp - rhs - 1] = '\0';
        Expr ip = { ixs, 0, strlen(ixs), env }; Value ix = parse_expr(&ip); check_expr_done(&ip);
        long long idx = as_num(&ix); v_free(&ix); free(ixs);
        const char *gt = rp + 1; while (*gt && isspace((unsigned char)*gt)) gt++;
        if (*gt != '>') fk_error("index assignment needs '>'");
        gt++; while (*gt && isspace((unsigned char)*gt)) gt++;
        Expr ep = { gt, 0, strlen(gt), env }; Value v = parse_expr(&ep); check_expr_done(&ep);
        sell_set(get_sell(env, name), idx, v);
        free(stmt); return;
    }

    fk_error("unsupported assignment: %s", stmt);
    free(stmt);
}

static void execute_source(Env *env, const char *src) {
    const char *p = src;
    const char *line_start = src;
    int brace_depth = 0;
    int bracket_depth = 0;
    int paren_depth = 0;
    while (*p) {
        if (*p == '"') { p = skip_string(p, p + strlen(p)); continue; }
        if (*p == '[') bracket_depth++;
        else if (*p == ']') bracket_depth--;
        else if (*p == '(') paren_depth++;
        else if (*p == ')') paren_depth--;
        else if (*p == '{') brace_depth++;
        else if (*p == '}') brace_depth--;

        if (*p == '\n' && brace_depth == 0 && bracket_depth == 0 && paren_depth == 0) {
            execute_statement_span(env, line_start, p);
            line_start = p + 1;
        }
        p++;
    }
    execute_statement_span(env, line_start, p);
}

static char *read_all(FILE *f) {
    size_t cap = 8192, n = 0;
    char *buf = (char *)xmalloc(cap);
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (n + 1 >= cap) { cap *= 2; buf = (char *)xrealloc(buf, cap); }
        buf[n++] = (char)c;
    }
    buf[n] = '\0';
    return buf;
}

static void free_env(Env *e) {
    for (size_t i = 0; i < e->count; ++i) {
        Sell *s = &e->sells[i];
        for (long long j = 0; j < s->size; ++j) v_free(&s->items[j]);
        free(s->items);
    }
    free(e->sells);
}

static const char *demo =
    "//;;## FK test\n"
    "\n"
    "holy = sell make 1~10\n"
    "\n"
    "holy = index(1) > 10\n"
    "holy = index(2) > 20\n"
    "holy = index(3) > holy = sell index(1) + holy = sell index(2)\n"
    "\n"
    "if holy = sell index(3) > 20 {\n"
    "    prinf(\"BIG\")\n"
    "}\n"
    "\n"
    "[prinf(\"FK\")](5)\n";

int main(int argc, char **argv) {
    Env env = {0};
    char *src = NULL;

    if (argc == 1) {
        src = xstrdup(demo);
    } else if (argc == 2) {
        FILE *f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return EXIT_FAILURE; }
        src = read_all(f); fclose(f);
    } else {
        fprintf(stderr, "usage: %s [file.fk]\n", argv[0]);
        return EXIT_FAILURE;
    }

    execute_source(&env, src);
    free(src);
    free_env(&env);
    return EXIT_SUCCESS;
}
