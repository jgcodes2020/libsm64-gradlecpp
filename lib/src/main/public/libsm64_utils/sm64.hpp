/**
 * @file sm64.hpp
 * @author your name (you@domain.com)
 * @brief Contains utilities to load and 
 * @version 0.1
 * @date 2021-05-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

//Include required library headers
#include "libsm64_utils/movie.hpp"

#ifndef __LIBSM64_SM64_HPP
#define __LIBSM64_SM64_HPP

#include <cstring>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <array>
#include <vector>
#include <stdexcept>

using std::string, std::array, std::vector;
using std::ios, std::fstream, std::ostringstream;

namespace libsm64 {
  /**
   * @brief Indicates an error to do with shared libraries.
   * Provides a field for a platform-specific error codes.
   */
  class shared_lib_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
  public:
    uint64_t error_code;
  };
  /**
   * @brief Represents a savestate.
   */
  class savestate {
  private:
    array<vector<uint8_t>, 2> handle;
  public:
    /**
     * @brief Retrieves a buffer from this savestate.
     *
     * @param i the index of the element to retrieve
     * @return the buffer
     */
    vector<uint8_t>& operator[](uint64_t i) {
      return handle[i];
    }
  };
  /**
   * @brief Represents the sm64 shared library and
   * provides a convenient interface for interacting with it.
   */
  class sm64 {
  public:
    /**
     * @brief Represents a version of libsm64.
     * @details The constants correspond to the "sm64_us.dll"
     * and "sm64_jp.dll" generated by Wafel, respectively.
     */
    enum class version : size_t {
      /**
       * @brief Represents the US version of libsm64.
       */
      US = 0,
      /**
       * @brief Reperesents the JP version of libsm64.
       */
      JP = 1
    };
    /**
     * @brief Represents a region of memory.
     */
    struct mem_region {
      /**
       * @brief Represents the virtual address of the region.
       */
      ptrdiff_t address;
      /**
       * @brief Represents the size of the region.
       */
      uintptr_t size;
    };
  private:
    void* _lib;
    version _version;
    array<mem_region, 2> _regions;
    /**
     * @brief Locates a symbol in the shared library.
     *
     * @param symbol a symbol in the shared library
     * @return a pointer to the requested symbol.
     */
    void* _impl_locate(string symbol);
  public:
    /**
     * @brief Loads libsm64 and sets up version information.
     *
     * @param path
     * @param version
     */
    sm64(string path, version version);
    ~sm64();

    /**
     * @brief Locates a global symbol, optionally adding a byte offset.
     *
     * @tparam T the type of the returned pointer
     * @param symbol a symbol present in libsm64
     * @param offset the offset from the symbol's address, defaults to 0
     * @return a pointer of the specified type, pointing to the specified symbol with the specified offset.
     */
    template<typename T = void>
    T* locate(string symbol, ptrdiff_t offset = 0) {
      return reinterpret_cast<T*>(
        reinterpret_cast<uint8_t*>(_impl_locate(symbol)) + offset
        );
    }
    /**
     * @brief Steps 1 frame forward.
     * @note It is the developer's responsibility to call sm64::set_input() beforehand.
     */
    void advance();
    /**
     * @brief Allocates a savestate buffer, based on
     * the version info provided at construction time.
     *
     * @return a savestate object, which starts out empty.
     */
    savestate allocate_slot() {
      savestate result;
      for (int i = 0; i < 2; i++) {
        result[i] = vector<uint8_t>(_regions[i].size);
      }
      return result;
    }
    /**
     * @brief Saves libsm64's current state to a savestate.
     *
     * @param save the savestate to save to
     */
    void save_slot(savestate& save) {
      uint8_t* _lib_ptr = reinterpret_cast<uint8_t*>(_lib);
      for (int i = 0; i < 2; i++) {
        mem_region segment = _regions[i];
        std::vector<uint8_t>& buffer = save[i];
        memmove(&buffer[0], (_lib_ptr + segment.address), segment.size);
      }
    }
    /**
     * @brief Loads a savestate into libsm64's current state.
     *
     * @param save the savestate to load from
     */
    void load_slot(savestate& save) {
      uint8_t* _lib_ptr = reinterpret_cast<uint8_t*>(_lib);
      for (int i = 0; i < 2; i++) {
        mem_region segment = _regions[i];
        std::vector<uint8_t>& buffer = save[i];
        memmove((_lib_ptr + segment.address), &buffer[0], segment.size);
      }
    }

    void set_input(uint16_t buttons, int8_t stick_x, int8_t stick_y) {
      *locate<uint16_t>("gControllerPads", 0) = buttons;
      *locate<int8_t>("gControllerPads", 2) = stick_x;
      *locate<int8_t>("gControllerPads", 3) = stick_y;
    }
    #ifdef __LIBSM64_MOVIE_HPP
    /**
     * @brief Sets the current input. You will need to include "libsm64/movie.hpp" to use this function.
     * 
     * @param input an input_frame with the needed input
     */
    void set_input(input_frame input) {
      set_input(input.m_buttons, input.m_stick_x, input.m_stick_y);
    }
    #endif

    /**
     * @brief Copies object behaviour from one slot to another.
     * 
     * @param src the object to copy from
     * @param dst the object to copy to
     */
    void copy_object(uint16_t src, uint16_t dst) {
      uint8_t* const src_ptr = locate<uint8_t>("gObjectPool", (src * 1392) + 160);
      uint8_t* const dst_ptr = locate<uint8_t>("gObjectPool", (dst * 1392) + 160);
      memmove(dst_ptr, src_ptr, 1232);
    }
  };
}

#endif