#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../vec.h"

static int reverse_ints(void* vec, int a, int b) {
    int* s = vec;
    return s[a] > s[b];
}

void test_vec_int() {
    int* ints = vec_make(sizeof(int), 10, 10);

    assert(vec_len(ints) == 10);

    for (int i = 0; i < 10; ++i) {
        ints[i] = i;
    }

    for (int i = 10; i < 21; ++i) {
        ints = vec_appendv(ints, i);
        assert(ints != NULL);
    }

    assert(vec_len(ints) == 21);

    for (int i = 0; i < 21; ++i) {
        assert(ints[i] == i);
    }

    int ends[2] = {-2, vec_len(ints) - 1};
    for (int e = 0; e < 2; ++e) {
        int* teens = vec_sub(ints, 10, ends[e]);
        for (int i = 0; i < vec_len(teens); ++i) {
            printf("%d ", teens[i]);
            assert(teens[i] == i + 10);
        }
        printf("\n");
        vec_del(teens);
    }

    assert(NULL == vec_sub(NULL, 1, 3));
    assert(NULL == vec_sub(ints, 4, 2));
    assert(NULL == vec_sub(ints, 30, 31));
    assert(NULL == vec_sub(ints, 3, -50));

    vec_sort(ints, reverse_ints);
    printf("-- Sorted:\n");
    for (int i = 0; i < vec_len(ints); ++i) {
        printf("%d ", ints[i]);
    }
    printf("\n");

    vec_del(ints);
}

void test_vec_char() {
    char* s = vec_make(sizeof(char), 0, 10);
    s = vec_appendnv(s, 13, 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\0');
    char* hello = vec_sub(s, 0, 5);

    assert(strcmp("hello world!", s) == 0);
    assert(strcmp("hello", hello) == 0);

    vec_sort_chars(hello);

    printf("%s\n", hello);

    vec_del(hello);
    vec_del(s);
}

void test_vec_vec() {
    char c = 'a';
    char** m = vec_make(sizeof(char*), 5, 5);
    for (int i = 0; i < 5; ++i) {
        m[i] = vec_make(sizeof(char), 5, 5);
        for (int j = 0; j < 5; ++j) {
            m[i][j] = c++;
        }
    }

    c = 'a';
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            printf("%c ", m[i][j]);
            assert(c++ == m[i][j]);
        }
        vec_del(m[i]);
        printf("\n");
    }
    vec_del(m);
}

void test_vec_string() {
    char** s = vec_make(sizeof(char*), 0, 10);
    s = vec_appendnv(s, 4, "Zinedine", "Vincent", "Alice", "Bob");

    vec_sort_cstrings(s);

    printf("== Sorted:\n");
    for (int i = 0; i < 4; ++i) {
        printf("%s\n", s[i]);
    }
}

typedef struct person {
    char* name;
    int age;
} person;

void print_person(const person* p) { printf("[%s age=%d]", p->name, p->age); }

static int sort_person(void* vec, int a, int b) {
    person* s = vec;
    if (strcmp(s[a].name, s[b].name) < 0) {
        return 1;
    }
    return s[a].age < s[b].age;
}

void test_vec_person() {
    person alice = {"Alice", 40};
    person alice2 = {"Alice", 21};
    person bob = {"Bob", 55};

    person* s = vec_make(sizeof(person), 0, 0);

    s = vec_appendnp(s, 3, &alice, &alice2, &bob);

    for (int i = 0; i < 3; ++i) {
        print_person(&s[i]);
        printf(" ");
    }
    printf("\n");

    vec_sort(s, sort_person);

    printf("--Sorted:\n");
    for (int i = 0; i < 3; ++i) {
        print_person(&s[i]);
        printf(" ");
    }
    printf("\n");

    vec_del(s);
}

struct print_ctx {
    int n;
    int len;
};

static void print_strmap_int(void* c, const char* key, void* value) { struct print_ctx* ctx = c; }

void test_strmap_int() {
    strmap_t m = strmap_make(sizeof(int), 20);
    assert(0 == strmap_len(m));

    int i = 0;
    m = strmap_addp(m, "zero", &i);
    i = 10;
    m = strmap_addp(m, "ten", &i);
    i = 3;
    m = strmap_addp(m, "three", &i);

    m = strmap_addv(m, "three", 33);
    m = strmap_addv(m, "forty two", 42);

    assert(4 == strmap_len(m));

    int ok = strmap_erase(m, "five");
    assert(!ok);
    ok = strmap_erase(m, "zero");
    assert(ok);
    assert(3 == strmap_len(m));

    assert(!strmap_contains(m, "vincent"));
    int v;
    ok = strmap_get(m, "ten", &v);
    assert(ok);
    assert(10 == v);
    ok = strmap_get(m, "three", &v);
    assert(ok);
    assert(33 == v);
    ok = strmap_get(m, "forty two", &v);
    assert(ok);
    assert(42 == v);

    struct print_ctx ctx;
    ctx.len = strmap_len(m);
    ctx.n = 0;
    for (strmap_iterator_t it = strmap_iterator(m); strmap_next(&it);) {
        const int* v = it.value;
        if (ctx.n == 0) {
            printf("{\n");
        }
        ++ctx.n;
        printf("  \"%s\": %d", it.key, *v);
        if (ctx.n == ctx.len) {
            printf("\n}\n");
        } else {
            printf(",\n");
        }
    }

    strmap_del(m);
}

void test_strmap_big() {
    strmap_t m = strmap_make(sizeof(int), 256);
    char s[2] = {0, 0};

    for (int i = 33; i < 126; ++i) {
        s[0] = i;
        assert(m = strmap_addv(m, s, 0));
    }

    assert(strmap_get(m, "f", NULL));

    strmap_del(m);
}

int main() {
    test_vec_int();
    test_vec_char();
    test_vec_vec();
    test_vec_string();
    test_vec_person();

    test_strmap_int();
    test_strmap_big();
}
