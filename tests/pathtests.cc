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
#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "doctest.h"
#include "path.h"

TEST_CASE("Path::ensuring_trailing_separator") {

    CHECK(Path::ensuring_trailing_separator("", Path::kEPOCSeparator) == "\\");
    CHECK(Path::ensuring_trailing_separator("\\", Path::kEPOCSeparator) == "\\");
    CHECK(Path::ensuring_trailing_separator("C:", Path::kEPOCSeparator) == "C:\\");
    CHECK(Path::ensuring_trailing_separator("C:\\", Path::kEPOCSeparator) == "C:\\");

    // N.B. These tests assume a POSIX host and will need updating for future Windows support.
    CHECK(Path::ensuring_trailing_separator("", Path::kHostSeparator) == "/");
    CHECK(Path::ensuring_trailing_separator("/", Path::kHostSeparator) == "/");
    CHECK(Path::ensuring_trailing_separator("/mnt", Path::kHostSeparator) == "/mnt/");
    CHECK(Path::ensuring_trailing_separator("/mnt/", Path::kHostSeparator) == "/mnt/");

    // Unsupported path normalization and separator conversion.
    CHECK(Path::ensuring_trailing_separator("/mnt/", Path::kEPOCSeparator) == "/mnt/\\");
    CHECK(Path::ensuring_trailing_separator("C:\\", Path::kHostSeparator) == "C:\\/");
    CHECK(Path::ensuring_trailing_separator("C:\\\\", Path::kEPOCSeparator) == "C:\\\\");
    CHECK(Path::ensuring_trailing_separator("/mnt//", Path::kHostSeparator) == "/mnt//");
}

TEST_CASE("Path::getEPOCBasename") {
    CHECK(Path::getEPOCBasename("C:\\Random") == "Random");
    CHECK(Path::getEPOCBasename("C:\\Documents\\foo.txt") == "foo.txt");

    // Legacy behavior (might be a bug).
    CHECK(Path::getEPOCBasename("C:\\Random\\") == "");
}

// TODO: Rename to get parent path?
TEST_CASE("Path::getEPOCDirname") {
    SUBCASE("C:\\Random") {
        std::string result;
        result = Path::getEPOCDirname("C:\\Random");
        CHECK(result == "C:\\");
    }
    SUBCASE("C:\\Random\\") {
        std::string result;
        result = Path::getEPOCDirname("C:\\Random\\");
        CHECK(result == "C:\\");
    }
    SUBCASE("C:\\Documents\\foo.txt") {
        std::string result;
        result = Path::getEPOCDirname("C:\\Documents\\foo.txt");
        CHECK(result == "C:\\Documents");
    }
    SUBCASE("C:\\") {
        std::string result;
        result = Path::getEPOCDirname("C:\\");
        CHECK(result == "C:");
    }

    // Legacy behavior (might be a bug).
    SUBCASE("C:") {
        std::string result;
        result = Path::getEPOCDirname("C:");
        CHECK(result == "C");
    }
}

TEST_CASE("Path::is_absolute") {

    CHECK(Path::is_absolute("C:\\", '\\') == true);
    CHECK(Path::is_absolute("C:\\", '/') == false);

    CHECK(Path::is_absolute("C:", '\\') == true);
    CHECK(Path::is_absolute("C:", '/') == false);

    CHECK(Path::is_absolute("", '\\') == false);
    CHECK(Path::is_absolute("", '/') == false);

    CHECK(Path::is_absolute("foo", '\\') == false);
    CHECK(Path::is_absolute("foo", '/') == false);
    CHECK(Path::is_absolute("foo\\bar", '\\') == false);
    CHECK(Path::is_absolute("foo/bar", '/') == false);

    CHECK(Path::is_absolute("\\C:\\", '\\') == false);
    CHECK(Path::is_absolute("/C:/", '/') == true);
}

TEST_CASE("Path::appending_components") {
    CHECK(Path::appending_components("C:\\", {"Documents"}, '\\') == "C:\\Documents");
    CHECK(Path::appending_components("C:\\", {"Documents"}, '\\') == "C:\\Documents");
}

TEST_CASE("Path::split") {
    CHECK(Path::split("", '\\') == std::vector<std::string>({}));
    CHECK(Path::split("one\\two\\three", '\\') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(Path::split("one\\two\\three\\", '\\') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(Path::split("one\\two\\\\three\\", '\\') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(Path::split("C:\\one\\two\\\\three\\", '\\') == std::vector<std::string>({"C:", "one", "two", "three"}));

    CHECK(Path::split("", '/') == std::vector<std::string>({}));
    CHECK(Path::split("one/two/three", '/') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(Path::split("one/two/three/", '/') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(Path::split("one/two//three/", '/') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(Path::split("/one/two/three/", '/') == std::vector<std::string>({"/", "one", "two", "three"}));
}

TEST_CASE("Path::join") {
    CHECK(Path::join({""}, '/') == "");
    CHECK(Path::join({"hello"}, '/') == "hello");
    CHECK(Path::join({"hello", "world"}, '/') == "hello/world");
    CHECK(Path::join({"/hello", "world"}, '/') == "/hello/world");
    CHECK(Path::join({"/", "hello", "world"}, '/') == "/hello/world");
    CHECK(Path::join({"hello", "world"}, '\\') == "hello\\world");
    CHECK(Path::join({"C:\\hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(Path::join({"C:\\", "hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(Path::join({"C:", "hello", "world"}, '\\') == "C:\\hello\\world");
}

TEST_CASE("Path::resolve_path") {

    // Posix.
    CHECK(Path::resolve_path("/foo/bar/baz", "", '/') == "/foo/bar/baz");
    CHECK(Path::resolve_path("/foo/bar/baz", "/one/two", '/') == "/foo/bar/baz");
    CHECK(Path::resolve_path("baz", "/foo/bar", '/') == "/foo/bar/baz");
    CHECK(Path::resolve_path("../baz", "/foo/bar", '/') == "/foo/baz");

    // Windows (and EPOC).
    CHECK(Path::resolve_path("C:\\foo\\bar\\baz", "", '\\') == "C:\\foo\\bar\\baz");
    CHECK(Path::resolve_path("C:\\foo\\bar\\baz", "C:\\one\\two", '\\') == "C:\\foo\\bar\\baz");
    CHECK(Path::resolve_path("baz", "C:\\foo\\bar", '\\') == "C:\\foo\\bar\\baz");
    CHECK(Path::resolve_path("..\\baz", "D:\\foo\\bar", '\\') == "D:\\foo\\baz");
}
