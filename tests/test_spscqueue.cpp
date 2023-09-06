/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>

#include "test.cpp"
#include "utils.cpp"

#include "spscqueue.cpp"


using namespace JS80P;


TEST(queue_is_created_empty, {
    SPSCQueue<std::string> q(16);

    assert_true(q.is_empty());
    assert_eq(0, q.length());
})


TEST(queue_is_lock_free, {
    SPSCQueue<std::string> q(16);

    assert_true(q.is_lock_free());
})


TEST(popping_from_empty_queue_fails, {
    SPSCQueue<std::string> q(16);
    std::string item("unchanged");

    assert_false(q.pop(item));
    assert_eq("unchanged", item);
})


TEST(pushed_item_is_on_top_and_can_be_popped, {
    SPSCQueue<std::string> q(16);
    std::string popped;

    assert_true(q.push("some-item"));
    assert_eq(1, q.length());
    assert_false(q.is_empty());
    assert_true(q.pop(popped));
    assert_true(q.is_empty());
    assert_eq("some-item", popped);
})


TEST(fifo, {
    SPSCQueue<std::string> q(16);
    std::string a, b, c, d, e;

    assert_true(q.push("a"));
    assert_true(q.push("b"));
    assert_true(q.push("c"));
    assert_true(q.push("d"));
    assert_true(q.push("e"));

    assert_eq(5, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(a));
    assert_eq(4, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(b));
    assert_eq(3, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(c));
    assert_eq(2, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(d));
    assert_eq(1, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(e));
    assert_eq(0, q.length());
    assert_true(q.is_empty());

    assert_eq("a", a);
    assert_eq("b", b);
    assert_eq("c", c);
    assert_eq("d", d);
    assert_eq("e", e);
})


TEST(pushing_into_full_queue_fails, {
    SPSCQueue<std::string> q(2);
    std::string a, b;

    assert_true(q.push("a"));
    assert_true(q.push("b"));
    assert_false(q.push("c"));

    assert_eq(2, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(a));
    assert_eq(1, q.length());
    assert_false(q.is_empty());

    assert_true(q.pop(b));
    assert_eq(0, q.length());
    assert_true(q.is_empty());

    assert_eq("a", a);
    assert_eq("b", b);
})


TEST(queue_can_be_filled_and_emptied_multiple_times, {
    constexpr size_t size = 8;

    SPSCQueue<std::string> q(size);
    std::string str("a");

    for (int i = 0; i != 10; ++i) {
        char c = 'a';

        for (size_t j = 0; j != size; ++j, ++c) {
            str[0] = c;
            q.push(str);
        }

        assert_false(q.push("x"));

        c = 'a';

        for (size_t j = 0; j != size; ++j, ++c) {
            assert_true(q.pop(str));
            assert_eq(c, str[0]);
        }

    }
})
