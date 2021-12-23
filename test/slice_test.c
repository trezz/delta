#include "../slice.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../sort.h"

static int reverse_ints(int* slice, int a, int b) { return slice[a] > slice[b]; }

void test_slice_int() {
    int* ints = slice_make(sizeof(int), 10, 10);

    assert(slice_len(ints) == 10);

    for (int i = 0; i < 10; ++i) {
        ints[i] = i;
    }

    for (int i = 10; i < 21; ++i) {
        ints = slice_add(ints, i);
        assert(ints != NULL);
    }

    assert(slice_len(ints) == 21);

    for (int i = 0; i < 21; ++i) {
        assert(ints[i] == i);
    }

    int ends[2] = {-2, slice_len(ints) - 1};
    for (int e = 0; e < 2; ++e) {
        int* teens = slice_sub(ints, 10, ends[e]);
        for (int i = 0; i < slice_len(teens); ++i) {
            printf("%d ", teens[i]);
            assert(teens[i] == i + 10);
        }
        printf("\n");
        slice_del(teens);
    }

    assert(NULL == slice_sub(NULL, 1, 3));
    assert(NULL == slice_sub(ints, 4, 2));
    assert(NULL == slice_sub(ints, 30, 31));
    assert(NULL == slice_sub(ints, 3, -50));

    sort_slice(ints, reverse_ints);
    printf("-- Sorted:\n");
    for (int i = 0; i < slice_len(ints); ++i) {
        printf("%d ", ints[i]);
    }
    printf("\n");

    slice_del(ints);
}

void test_slice_char() {
    char* s = slice_make(sizeof(char), 0, 10);
    s = slice_addn(s, 13, 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\0');
    char* hello = slice_sub(s, 0, 5);

    assert(strcmp("hello world!", s) == 0);
    assert(strcmp("hello", hello) == 0);

    sort_chars(hello);

    printf("%s\n", hello);

    slice_del(hello);
    slice_del(s);
}

void test_slice_slice() {
    char c = 'a';
    char** m = slice_make(sizeof(char*), 5, 5);
    for (int i = 0; i < 5; ++i) {
        m[i] = slice_make(sizeof(char), 5, 5);
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
        slice_del(m[i]);
        printf("\n");
    }
    slice_del(m);
}

void test_slice_string() {
    char** s = slice_make(sizeof(char*), 0, 10);
    s = slice_addn(s, 4, "Zinedine", "Vincent", "Alice", "Bob");

    sort_cstrings(s);

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

int sort_person(person* s, int a, int b) {
    if (strcmp(s[a].name, s[b].name) < 0) {
        return 1;
    }
    return s[a].age < s[b].age;
}

void test_slice_person() {
    person alice = {"Alice", 40};
    person alice2 = {"Alice", 21};
    person bob = {"Bob", 55};

    person* s = slice_make(sizeof(person), 0, 0);

    s = slice_storen(s, 3, &alice, &alice2, &bob);

    for (int i = 0; i < 3; ++i) {
        print_person(&s[i]);
        printf(" ");
    }
    printf("\n");

    sort_slice(s, sort_person);

    printf("--Sorted:\n");
    for (int i = 0; i < 3; ++i) {
        print_person(&s[i]);
        printf(" ");
    }
    printf("\n");

    slice_del(s);
}

struct print_ctx {
    int n;
    int len;
};

static void print_map_int(void* c, const char* key, const void* value) {
    struct print_ctx* ctx = c;
    const int* v = value;
    if (ctx->n == 0) {
        printf("{\n");
    }
    ++ctx->n;
    printf("  \"%s\": %d", key, *v);
    if (ctx->n == ctx->len) {
        printf("\n}\n");
    } else {
        printf(",\n");
    }
}

void test_map_int() {
    map_t m = map_make(sizeof(int), 20);
    assert(0 == map_len(m));

    int i = 0;
    map_store(m, "zero", &i);
    i = 10;
    map_store(m, "ten", &i);
    i = 3;
    map_store(m, "three", &i);

    map_add(m, "three", 33);
    map_add(m, "forty two", 42);

    assert(4 == map_len(m));

    int ok = map_erase(m, "five");
    assert(!ok);
    ok = map_erase(m, "zero");
    assert(ok);
    assert(3 == map_len(m));

    assert(!map_contains(m, "vincent"));
    int v;
    ok = map_get(m, "ten", &v);
    assert(ok);
    assert(10 == v);
    ok = map_get(m, "three", &v);
    assert(ok);
    assert(33 == v);
    ok = map_get(m, "forty two", &v);
    assert(ok);
    assert(42 == v);

    struct print_ctx ctx;
    ctx.len = map_len(m);
    ctx.n = 0;
    map_each_ctx(m, print_map_int, &ctx);

    map_print_internals(m);

    map_del(m);
}

void test_map_big() {
    map_t m = map_make(sizeof(int), 256);
    char s[2] = {0, 0};

    for (int i = 33; i < 126; ++i) {
        s[0] = i;
        assert(map_add(m, s, 0));
    }

    map_print_internals(m);

    assert(map_get(m, "f", NULL));

    map_del(m);
}

int main() {
    test_slice_int();
    test_slice_char();
    test_slice_slice();
    test_slice_string();
    test_slice_person();

    test_map_int();
    test_map_big();
}
