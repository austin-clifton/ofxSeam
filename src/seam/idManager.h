#pragma once

#include <cstdint>
#include <atomic>

namespace seam {
    /// @brief A non-random, simple incremental ID distributor with 8-byte IDs.
    /// Contains member functions to retrieve the next ID,
    /// increment the next available ID to a higher ID, and reset to the first available ID (1).
    /// This class does NOT generate UUIDs.
    class IdManager {
    public:
        /// @brief Should be called when an ID is wanted. 
        /// The next available ID will be increased.
        /// Can be safely called from multiple threads.
        size_t RetrieveNext();

        /// @brief Should be called when ID tracking should be restarted at 1.
        void Reset();

        /// @brief Should be called whenever an ID is manually assigned, 
        /// for instance, during de-serialization, to make sure IDs aren't re-used.
        /// @param id 
        void SetNextAvailableId(size_t id);

    private:
        std::atomic<size_t> nextAvailableId = 1;
    };
}