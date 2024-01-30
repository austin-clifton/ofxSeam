#pragma once

#include "seam/idManager.h"

namespace seam {
    /// @brief Singleton for distributing Seam Pin and Node IDs.
    /// See: https://stackoverflow.com/a/1008289
    class IdsDistributor {
        public:
            IdsDistributor(IdsDistributor const&) = delete;
            void operator=(IdsDistributor const&) = delete;

            static IdsDistributor& GetInstance() {
                static IdsDistributor instance;
                return instance;
            }

            inline size_t NextNodeId() {
                return nodeIdManager.RetrieveNext();
            }

            inline size_t NextPinId() {
                return pinIdManager.RetrieveNext();
            }

            inline void SetNextPinId(size_t id) {
                pinIdManager.SetNextAvailableId(id);
            }

            inline void SetNextNodeId(size_t id) {
                nodeIdManager.SetNextAvailableId(id);
            }

            inline void ResetIds() {
                nodeIdManager.Reset();
                pinIdManager.Reset();
            }
            
        private:
            IdsDistributor() { }

            IdManager nodeIdManager;
            IdManager pinIdManager;
    };
}