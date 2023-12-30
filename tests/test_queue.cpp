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

#include "test.cpp"
#include "utils.cpp"

#include "dsp/queue.cpp"


using namespace JS80P;


class TestObj
{
    public:
        TestObj(int const v = 0) : value(v)
        {
        }

        TestObj(TestObj const& o) = default;
        TestObj& operator=(TestObj const& o) = default;

        int value;
};


class TestObjQueue : public Queue<TestObj>
{
    public:
        TestObjQueue(Queue<TestObj>::SizeType const capacity = DEFAULT_CAPACITY)
            : Queue<TestObj>(capacity)
        {
        }

        Queue<TestObj>::SizeType capacity() const
        {
            return items.capacity();
        }
};


TEST(newly_created_queue_can_allocate_memory_for_the_given_number_of_items, {
    constexpr Queue<TestObj>::SizeType capacity = 128;
    TestObjQueue q(capacity);

    assert_true(q.is_empty());
    assert_eq(0, q.length());
    assert_eq((int)capacity, (int)q.capacity());
})


TEST(pushed_item_is_on_top_and_can_be_popped, {
    Queue<TestObj> q;
    TestObj item(123);

    q.push(item);
    assert_false(q.is_empty());

    assert_eq(1, q.length());
    assert_eq(123, q.front().value);
    assert_eq(123, q.pop().value);
})


TEST(fifo, {
    Queue<TestObj> q;
    TestObj a(1);
    TestObj b(2);
    TestObj c(3);
    TestObj d(4);
    TestObj e(5);

    q.push(a);
    assert_eq(1, q.back().value);

    q.push(b);
    assert_eq(2, q.back().value);

    q.push(c);
    assert_eq(3, q.back().value);

    q.push(d);
    assert_eq(4, q.back().value);

    q.push(e);
    assert_eq(5, q.back().value);

    assert_eq(5, q.length());
    assert_eq(1, q.front().value);
    assert_eq(1, q.pop().value);

    assert_eq(4, q.length());
    assert_eq(2, q.front().value);
    assert_eq(2, q.pop().value);

    assert_eq(3, q.length());
    assert_eq(3, q.front().value);
    assert_eq(3, q.pop().value);

    assert_eq(2, q.length());
    assert_eq(4, q.front().value);
    assert_eq(4, q.pop().value);

    assert_eq(1, q.length());
    assert_eq(5, q.front().value);
    assert_eq(5, q.pop().value);

    assert_eq(0, q.length());
    assert_true(q.is_empty());
})


TEST(increases_capacity_when_necessary, {
    constexpr int count = 16;
    TestObjQueue q;

    for (int i = 0; i != count; ++i) {
        TestObj item(i);
        q.push(item);
    }

    assert_eq(count, q.length());
    assert_gte((int)q.capacity(), count);

    for (int i = 0; i != count; ++i) {
        assert_eq(i, q.pop().value);
    }

    assert_eq(0, q.length());
    assert_gte((int)q.capacity(), count);
})


TEST(when_becomes_empty_then_resets, {
    constexpr int count = 16;
    TestObjQueue q;

    for (int i = 0; i != count; ++i) {
        TestObj item(i);
        q.push(item);
    }

    for (int i = 0; i != count; ++i) {
        assert_eq(i, q.pop().value);
    }

    assert_eq(0, q.length());
    assert_eq(count, (int)q.capacity());

    for (int i = 0; i != count; ++i) {
        TestObj item(i + count);
        q.push(item);
    }

    assert_eq(count, q.length());
    assert_eq(count, (int)q.capacity());

    for (int i = 0; i != count; ++i) {
        assert_eq(i + count, q.pop().value);
    }

    assert_eq(0, q.length());
    assert_eq(count, (int)q.capacity());
})





TEST(elements_may_be_accessed_randomly, {
    TestObjQueue q;

    q.push(10);
    q.push(20);
    q.push(30);
    q.push(40);
    q.push(50);

    assert_eq(10, q[0].value);
    assert_eq(10, q.pop().value);

    assert_eq(20, q[0].value);
    assert_eq(20, q.pop().value);

    assert_eq(30, q[0].value);
    assert_eq(40, q[1].value);
    assert_eq(50, q[2].value);
})


TEST(elements_may_be_dropped_after_a_given_index, {
    TestObjQueue q;

    q.push(10);
    q.push(20);
    q.push(30);
    q.push(40);
    q.push(50);
    q.push(60);

    q.pop();
    q.drop(2);

    assert_eq(2, q.length());
    assert_eq(20, q[0].value);
    assert_eq(30, q[1].value);
    assert_eq(30, q.back().value);
})


TEST(the_entire_queue_may_be_dropped, {
    TestObjQueue q;

    q.push(10);
    q.push(20);
    q.push(30);

    q.drop(0);

    assert_eq(0, q.length());
    assert_true(q.is_empty());
})
