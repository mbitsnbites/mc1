// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include "demo_select.h"

#include <mc1/keyboard.h>
#include <mc1/leds.h>
#include <mc1/memory.h>
#include <mc1/mmio.h>
#include <mc1/vconsole.h>

#include <cstring>
#include <random>
#include <mr32intrin.h>

#ifdef ENABLE_SELFTEST
// Defined by libselftest.
extern "C" int selftest_run(void (*callback)(const int));
#endif

#ifdef ENABLE_DHRYSTONE
// Defined by dhry_1.c.
extern "C" void dhrystone(int Number_Of_Runs);
#endif

// Defined by the linker script.
extern char __rom_size;
extern char __bss_start;
extern char __bss_size;

namespace {
inline uint32_t linker_constant(const char* ptr) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr));
}

#ifdef ENABLE_DHRYSTONE
struct clkticks_t {
  uint32_t hi;
  uint32_t lo;
};

clkticks_t get_ticks() {
  clkticks_t clkticks;
  clkticks.hi = MMIO(CLKCNTHI);
  while (true) {
    clkticks.lo = MMIO(CLKCNTLO);
    const uint32_t new_cnthi = MMIO(CLKCNTHI);
    if (new_cnthi == clkticks.hi) {
      break;
    }
    clkticks.hi = new_cnthi;
  }
  return clkticks;
}

float elapsed_seconds(const clkticks_t start, const clkticks_t end) {
  // Caclulate the clock tick difference.
  const uint32_t hicorr = (start.lo > end.lo) ? 1 : 0;
  const uint32_t dhi = end.hi - start.hi + hicorr;
  const uint32_t dlo = end.lo - start.lo;
  const float delta = _mr32_utof(dhi, 32) + _mr32_utof(dlo, 0);

  // Get the CPU clock frequency (ticks per second).
  const float ticks_per_s = _mr32_utof(MMIO(CPUCLK), 0);

  // Return the time in seconds.
  return delta / ticks_per_s;
}
#endif

#ifdef ENABLE_SELFTEST
void selftest_callback(const int ok) {
  vcon_print(ok ? "*" : "!");
}
#endif

template <int N>
void print_dec_times_N(const int x_times_N) {
  vcon_print_dec(x_times_N / N);
  vcon_print(".");
  vcon_print_dec(x_times_N % N);
}

void print_size(uint32_t size) {
  static const char* SIZE_SUFFIX[] = {" bytes", " KB", " MB", " GB"};
  int size_div = 0;
  while (size >= 1024u && (size & 1023u) == 0u) {
    size = size >> 10;
    ++size_div;
  }
  vcon_print_dec(static_cast<int>(size));
  vcon_print(SIZE_SUFFIX[size_div]);
}

void print_addr_and_size(const char* str, const uint32_t addr, const uint32_t size) {
  vcon_print(str);
  vcon_print("0x");
  vcon_print_hex(addr);
  vcon_print(", ");
  print_size(size);
  vcon_print("\n");
}

#ifdef ENABLE_RAM_TEST
void run_ram_test(volatile uint32_t* mem_start,
                  volatile uint32_t* mem_end,
                  const int inner_passes,
                  const int outer_passes) {
  {
    vcon_print("  Word test: [");
    static const uint32_t TEST_DATA[] = {0x12345678, 0x11223344, 0xdeadbeef};
    volatile auto* ptr = reinterpret_cast<volatile uint32_t*>(mem_start);
    for (unsigned i = 0; i < sizeof(TEST_DATA) / sizeof(TEST_DATA[0]); ++i) {
      ptr[i] = TEST_DATA[i];
    }
    for (unsigned i = 0; i < sizeof(TEST_DATA) / sizeof(TEST_DATA[0]); ++i) {
      const auto x = ptr[i];
      if (i > 0)
        vcon_print(", ");
      vcon_print("0x");
      vcon_print_hex(x);
    }
    vcon_print("]\n");
  }

  {
    vcon_print("  Byte test: [");
    static const char TEST_DATA[] = "0123456789abcdef This is a test string!";
    volatile auto* ptr = reinterpret_cast<volatile char*>(mem_start);
    for (unsigned i = 0; i < sizeof(TEST_DATA); ++i) {
      ptr[i] = TEST_DATA[i];
    }
    for (unsigned i = 0; i < sizeof(TEST_DATA); ++i) {
      char str[2];
      str[0] = ptr[i];
      str[1] = 0;
      vcon_print(str);
    }
    vcon_print("]\n");
  }

  vcon_print("  Random test: [");
  int total_fails = 0;
  for (int pass = 0; pass < outer_passes; ++pass) {
    auto seed = pass * 12345;
    int num_fails = 0;

    for (int i = 0; i < inner_passes; ++i) {
      // Write...
      {
        std::mt19937 prng(seed);
        for (auto ptr = mem_start; ptr < mem_end; ++ptr)
          *ptr = static_cast<uint32_t>(prng());
      }

      // Read and check...
      {
        std::mt19937 prng(seed);
        for (auto ptr = mem_start; ptr < mem_end; ++ptr) {
          const auto x = *ptr;
          const auto expected = static_cast<uint32_t>(prng());
          if (x != expected)
            ++num_fails;
        }
      }
    }

    if (pass > 0)
      vcon_print(", ");
    vcon_print_dec(num_fails);
    total_fails += num_fails;
  }
  vcon_print("]: ");
  vcon_print_dec(total_fails);
  vcon_print(" / ");
  vcon_print_dec(inner_passes * outer_passes * static_cast<int>(mem_end - mem_start));
  vcon_print(" errors\n\n");
}

void run_xram_test() {
  const auto xram_size = MMIO(XRAMSIZE);
  if (xram_size == 0u)
    return;

  vcon_print("XRAM test:\n");

  // Pick an area of the XRAM that is likely to be unpopulated by data (e.g. data structures from
  // the memory allocator).
  volatile auto* mem_start = reinterpret_cast<uint32_t*>(XRAM_START + 2 * xram_size / 4);
  volatile auto* mem_end = reinterpret_cast<uint32_t*>(XRAM_START + 3 * xram_size / 4);

  run_ram_test(mem_start, mem_end, 2, 5);
}

void run_vram_test() {
  vcon_print("VRAM test:\n");
  // Allocate a reasonable size of VRAM for testing.
  auto vram_size = mem_query_free(MEM_TYPE_VIDEO);
  while (vram_size > 0u) {
    // Try to allocate a continous block of the given size.
    volatile auto* mem_start = reinterpret_cast<uint32_t*>(mem_alloc(vram_size, MEM_TYPE_VIDEO));
    if (mem_start != nullptr) {
      // Run the memory test.
      volatile auto* mem_end = &mem_start[vram_size/4];
      run_ram_test(mem_start, mem_end, 50, 5);

      // Done!
      break;
    }

    // Try a smaller block size (75% of the previous size).
    vram_size = (3 * vram_size) >> 2;
  }
}
#endif  // ENABLE_RAM_TEST

class console_t {
public:
  void init();
  void de_init();
  void draw(const int frame_no);

private:
  static const int MAX_COMMAND_LEN = 127;

  void* m_vcon_mem;
  char m_command[MAX_COMMAND_LEN + 1];
  int m_command_pos;
};

void console_t::init() {
  if (m_vcon_mem != NULL) {
    return;
  }

  // Allocate memory for the video console framebuffer.
  const unsigned size = vcon_memory_requirement();
  m_vcon_mem = mem_alloc(size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (m_vcon_mem == NULL) {
    return;
  }

  // Show the console.
  vcon_init(m_vcon_mem);
  vcon_show(LAYER_1);
  vcon_print("\n                      **** MC1 - The MRISC32 computer ****\n\n");

  // Print some memory information etc.
  print_addr_and_size("ROM:      ", ROM_START, linker_constant(&__rom_size));
  print_addr_and_size("VRAM:     ", VRAM_START, MMIO(VRAMSIZE));
  print_addr_and_size("XRAM:     ", XRAM_START, MMIO(XRAMSIZE));
  print_addr_and_size("\nbss:      ", linker_constant(&__bss_start), linker_constant(&__bss_size));

  // Print CPU info.
  vcon_print("\n\nCPU Freq: ");
  print_dec_times_N<10>((static_cast<int>(MMIO(CPUCLK)) + 50000) / 100000);
  vcon_print(" MHz\n\n");

#ifdef ENABLE_SELFTEST
  // Run the selftest.
  vcon_print("Selftest: ");
  if (selftest_run(selftest_callback)) {
    vcon_print(" PASS\n\n");
  } else {
    vcon_print(" FAIL\n\n");
  }
#endif

#ifdef ENABLE_DHRYSTONE
  {
    // Run the Dhrystone benchmark.
    vcon_print("Dhrystone: ");
    const int number_of_runs = 100000;
    const auto start_time = get_ticks();
    dhrystone(number_of_runs);
    const auto end_time = get_ticks();
    const auto user_time = elapsed_seconds(start_time, end_time);

    // Calculate metrics:
    //  1) Dhrystones per second
    //  2) DMIPS (relative to VAX 11/780)
    //  3) DMIPS/MHz (relative to CPU frequency)
    const auto dhrystones_per_second = static_cast<float>(number_of_runs) / user_time;
    const auto dmips = dhrystones_per_second / 1757.0f;
    const auto dmips_per_mhz = (dmips * 1000000.0f) / static_cast<float>(MMIO(CPUCLK));

    // Print results.
    print_dec_times_N<10>(_mr32_ftoir(dhrystones_per_second * 10.0f, 0));
    vcon_print(" Dhrystones/s, ");
    print_dec_times_N<10>(_mr32_ftoir(dmips * 10.0f, 0));
    vcon_print(" DMIPS, ");
    print_dec_times_N<100>(_mr32_ftoir(dmips_per_mhz * 100.0f, 0));
    vcon_print(" DMIPS/MHz\n\n");
  }
#endif

#ifdef ENABLE_RAM_TEST
  run_xram_test();
  run_vram_test();
#endif

  // Give instructions.
  vcon_print("Use switches to select demo...\n\n\n");

  m_command_pos = 0;
}

void console_t::de_init() {
  if (m_vcon_mem == NULL) {
    return;
  }
  vcp_set_prg(LAYER_1, NULL);
  mem_free(m_vcon_mem);
  m_vcon_mem = NULL;
}

void console_t::draw(const int frame_no) {
  // TODO(m): Can we do anything interesting here?
  (void)frame_no;
  sevseg_print("OLLEH");  // Print a friendly "HELLO".

  // Print character events from the keyboard.
  while (auto event = kb_get_next_event()) {
    if (kb_event_is_press(event)) {
      const auto character = kb_event_to_char(event);
      if (character != 0) {
        const char str[2] = {static_cast<char>(character), 0};
        vcon_print(str);

        if (kb_event_scancode(event) != KB_ENTER) {
          if (m_command_pos < MAX_COMMAND_LEN) {
            m_command[m_command_pos++] = static_cast<char>(character);
          }
        } else {
          m_command[m_command_pos] = 0;
          m_command_pos = 0;

          if (std::strcmp(&m_command[0], "go mandelbrot") == 0) {
            g_demo_select = DEMO_MANDELBROT;
          } else if (std::strcmp(&m_command[0], "go raytrace") == 0) {
            g_demo_select = DEMO_RAYTRACE;
          } else if (std::strcmp(&m_command[0], "go retro") == 0) {
            g_demo_select = DEMO_RETRO;
          }
        }
      }
    }
  }
}

console_t s_console;

}  // namespace

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

extern "C" void console_init() {
  s_console.init();
}

extern "C" void console_deinit() {
  s_console.de_init();
}

extern "C" void console(const int frame_no) {
  s_console.draw(frame_no);
}
