/*
 * This file is part of plptools.
 *
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <memory>

template<typename T, typename E>
class Result {
public:

    template<typename F>
    static Result check(E success, F&& f) {
        T out{};
        E error = f( out);
        if (error != success) {
            return Result<T, E>::failure(error);
        }
        return Result<T, E>::success(std::move(out));
    }

    static Result success(std::unique_ptr<T> value) {
        return Result(std::move(value), nullptr);
    }

    static Result success(T value) {
        return Result(std::make_unique<T>(std::move(value)), nullptr);
    }

    static Result failure(E error) {
        return Result(nullptr, std::make_unique<E>(std::move(error)));
    }

    explicit operator bool() const {
        return static_cast<bool>(value_);
    }

    T& value() {
        assert(value_);
        return *value_;
    }

    const T& value() const {
        assert(value_);
        return *value_;
    }

    std::unique_ptr<T> takeValue() {
        assert(value_);
        return std::move(value_);
    }

    const E& error() const {
        assert(error_);
        return *error_;
    }

private:

    Result(std::unique_ptr<T> value, std::unique_ptr<E> error)
    : value_(std::move(value))
    , error_(std::move(error)) {}

    std::unique_ptr<T> value_;
    std::unique_ptr<E> error_;
};
