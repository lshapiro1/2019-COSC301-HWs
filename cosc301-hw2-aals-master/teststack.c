#include <stdio.h>
#include <assert.h>

#include "stack.h"

int main() {
    struct stack s;
    init(&s);
    assert(empty(&s));

    int last = 0;
    for (int i = last; i < 10; i += 2) {
      push(&s, i);
      assert(peek(&s) == i);
      assert(!empty(&s));
      last = i;
    }

    while (!empty(&s)) {
      int val = pop(&s);
      assert(val == last);
      last -= 2;
    }

    assert(empty(&s));

    printf("If you got here without the test program crashing, all the tests passed!\n");
    return 0;
}
