/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "basic_test.h"
#include "test_init.h"
#include "labstor/data_structures/thread_safe/unordered_map.h"
#include "labstor/data_structures/string.h"
#include "labstor/memory/allocator/page_allocator.h"

using labstor::ipc::MemoryBackendType;
using labstor::ipc::MemoryBackend;
using labstor::ipc::allocator_id_t;
using labstor::ipc::AllocatorType;
using labstor::ipc::Allocator;
using labstor::ipc::MemoryManager;
using labstor::ipc::Pointer;
using labstor::ipc::unordered_map;
using labstor::ipc::string;

void UnorderedMapOfIntIntTest() {
  Allocator *alloc = alloc_g;
  unordered_map<int, int> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  {
    for (int i = 0; i < 20; ++i) {
      map.emplace(i, i);
    }
  }

  // Check if the 20 entries are indexable
  {
    for (int i = 0; i < 20; ++i) {
      REQUIRE(map[i] == i);
    }
  }

  // Check if 20 entries are findable
  {
    for (int i = 0; i < 20; ++i) {
      auto iter = map.find(i);
      auto entry = *iter;
      REQUIRE(*entry.val_ == i);
    }
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto entry : map) {
      REQUIRE((0 <= *entry.key_ && *entry.key_ < 20));
      REQUIRE((0 <= *entry.val_ && *entry.val_ < 20));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Re-emplace elements
  {
    for (int i = 0; i < 20; ++i) {
      map.emplace(i, i+100);
      REQUIRE(map[i] == i+100);
    }
  }

  // Remove 15 entries from the map
  {
    for (int i = 0; i < 15; ++i) {
      map.erase(i);
    }
    REQUIRE(map.size() == 5);
    for (int i = 0; i < 15; ++i) {
      REQUIRE(map.find(i) == map.end());
    }
  }

  // Attempt to replace an existing key
  {
    for (int i = 15; i < 20; ++i) {
      REQUIRE(map.try_emplace(i, 100) == false);
    }
    for (int i = 15; i < 20; ++i) {
      REQUIRE(map[i] != 100);
    }
  }

  // Erase the entire map
  {
    map.clear();
    REQUIRE(map.size() == 0);
  }

  // Add 100 entries to the map (should force a growth)
  {
    for (int i = 0; i < 100; ++i) {
      map.emplace(i, i);
    }
    for (int i = 0; i < 100; ++i) {
      REQUIRE(map.find(i) != map.end());
    }
  }
}

void UnorderedMapOfIntStringTest() {
  Allocator *alloc = alloc_g;
  unordered_map<int, string> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  {
    for (int i = 0; i < 20; ++i) {
      int t1 = i;
      // auto t2 = string(std::to_string(i+1));
      map.emplace(t1, std::to_string(i+1));
    }
  }

  // Check if the 20 entries are indexable
  {
    for (int i = 0; i < 20; ++i) {
      int t1 = i;
      string t2(std::to_string(i+1));
      auto t3 = map[t1];
      REQUIRE(t3.str() == std::to_string(i+1));
    }
  }

  // Check if 20 entries are findable
  {
    for (int i = 0; i < 20; ++i) {
      int t1 = i;
      auto iter = map.find(t1);
      auto entry = *iter;
      REQUIRE(*entry.val_ == std::to_string(i+1));
    }
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto entry : map) {
      int key = *entry.key_;
      int val;
      std::stringstream((*entry.val_).str()) >> val;
      REQUIRE((0 <= key && key < 20));
      REQUIRE((1 <= val && val < 21));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Modify the fourth map entry (move assignment)
  {
    auto iter = map.find(4);
    auto val = (*iter).val_;
    val = std::move(string("25"));
    REQUIRE((*val) == "25");
  }

  // Verify the modification took place
  {
    REQUIRE(map[4] == "25");
  }

  // Modify the fourth map entry (copy assignment)
  {
    auto iter = map.find(4);
    auto val = (*iter).val_;
    string text("50");
    val = text;
    REQUIRE((*val) == "50");
  }

  // Verify the modification took place
  {
    REQUIRE(map[4] == "50");
  }

  // Remove 15 entries from the map
  {
    for (int i = 0; i < 15; ++i) {
      map.erase(i);
    }
    REQUIRE(map.size() == 5);
    for (int i = 0; i < 15; ++i) {
      REQUIRE(map.find(i) == map.end());
    }
  }

  // Erase the entire map
  {
    map.clear();
    REQUIRE(map.size() == 0);
  }

  // Add 100 entries to the map (will force a growth)
  {
    for (int i = 0; i < 100; ++i) {
      map.emplace(i, i);
    }
    for (int i = 0; i < 100; ++i) {
      REQUIRE(map.find(i) != map.end());
    }
  }
}

void UnorderedMapOfStringIntTest() {
  Allocator *alloc = alloc_g;
  unordered_map<string, int> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  {
    for (int i = 0; i < 20; ++i) {
      auto t1 = string(std::to_string(i));
      map.emplace(t1, i+1);
    }
  }

  // Check if the 20 entries are indexable
  {
    for (int i = 0; i < 20; ++i) {
      string t1(std::to_string(i));
      auto t3 = map[t1];
      REQUIRE(t3 == i+1);
    }
  }

  // Check if 20 entries are findable
  {
    for (int i = 0; i < 20; ++i) {
      string t1(std::to_string(i));
      auto iter = map.find(t1);
      auto entry = *iter;
      REQUIRE(*entry.val_ == i+1);
    }
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto entry : map) {
      int key;
      int val;
      std::stringstream((*entry.key_).str()) >> key;
      REQUIRE((0 <= key && key < 20));
      REQUIRE((1 <= *entry.val_ && *entry.val_ < 21));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Remove 15 entries from the map
  {
    for (int i = 0; i < 15; ++i) {
      string i_text(std::to_string(i));
      map.erase(i_text);
    }
    REQUIRE(map.size() == 5);
    for (int i = 0; i < 15; ++i) {
      string i_text(std::to_string(i));
      REQUIRE(map.find(i_text) == map.end());
    }
  }

  // Erase the entire map
  {
    map.clear();
    REQUIRE(map.size() == 0);
  }

  // Add 100 entries to the map (will force a growth)
  {
    for (int i = 0; i < 100; ++i) {
      string i_text(std::to_string(i));
      map.emplace(i_text, i);
    }
    for (int i = 0; i < 100; ++i) {
      string i_text(std::to_string(i));
      REQUIRE(map.find(i_text) != map.end());
    }
  }
}

void UnorderedMapOfStringStringTest() {
  Allocator *alloc = alloc_g;
  unordered_map<string, string> map(alloc);

  // Insert 20 entries into the map (no growth trigger)
  {
    for (int i = 0; i < 20; ++i) {
      auto t1 = string(std::to_string(i));
      auto t2 = string(std::to_string(i + 1));
      map.emplace(t1, t2);
    }
  }

  // Check if the 20 entries are indexable
  {
    for (int i = 0; i < 20; ++i) {
      string t1(std::to_string(i));
      string t2(std::to_string(i + 1));
      auto t3 = map[t1];
      REQUIRE(t3 == t2);
    }
  }

  // Check if 20 entries are findable
  {
    for (int i = 0; i < 20; ++i) {
      string t1(std::to_string(i));
      string t2(std::to_string(i + 1));
      auto iter = map.find(t1);
      auto entry = *iter;
      REQUIRE(*entry.val_ == t2);
    }
  }

  // Iterate over the map
  {
    auto prep = map.iter_prep();
    prep.Lock();
    int i = 0;
    for (auto entry : map) {
      int key;
      int val;
      std::stringstream((*entry.key_).str()) >> key;
      std::stringstream((*entry.val_).str()) >> val;
      REQUIRE((0 <= key && key < 20));
      REQUIRE((1 <= val && val < 21));
      ++i;
    }
    REQUIRE(i == 20);
  }

  // Modify the fourth map entry (move assignment)
  {
    auto iter = map.find(string("4"));
    auto val = (*iter).val_;
    val = std::move(string("25"));
    REQUIRE((*val) == "25");
  }

  // Verify the modification took place
  {
    REQUIRE(map[string("4")] == "25");
  }

  // Modify the fourth map entry (copy assignment)
  {
    auto iter = map.find(string("4"));
    auto val = (*iter).val_;
    string text("50");
    val = text;
    REQUIRE((*val) == "50");
  }

  // Verify the modification took place
  {
    REQUIRE(map[string("4")] == "50");
  }

  // Remove 15 entries from the map
  {
    for (int i = 0; i < 15; ++i) {
      string i_text(std::to_string(i));
      map.erase(i_text);
    }
    REQUIRE(map.size() == 5);
    for (int i = 0; i < 15; ++i) {
      string i_text(std::to_string(i));
      REQUIRE(map.find(i_text) == map.end());
    }
  }

  // Erase the entire map
  {
    map.clear();
    REQUIRE(map.size() == 0);
  }

  // Add 100 entries to the map (will force a growth)
  {
    for (int i = 0; i < 100; ++i) {
      string i_text(std::to_string(i));
      map.emplace(i_text, i_text);
    }
    for (int i = 0; i < 100; ++i) {
      string i_text(std::to_string(i));
      REQUIRE(map.find(i_text) != map.end());
    }
  }
}

TEST_CASE("UnorderedMapOfIntInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UnorderedMapOfIntIntTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("UnorderedMapOfIntString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UnorderedMapOfIntStringTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}


TEST_CASE("UnorderedMapOfStringInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UnorderedMapOfStringIntTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("UnorderedMapOfStringString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  UnorderedMapOfStringStringTest();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
