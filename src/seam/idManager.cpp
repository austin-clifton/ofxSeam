#include "idManager.h"

using namespace seam;

size_t IdManager::RetrieveNext() {
    return nextAvailableId.fetch_add(1);
}

void IdManager::Reset() {
    nextAvailableId = 1;
}

void IdManager::SetNextAvailableId(size_t id) {
    if (id > nextAvailableId) {
        nextAvailableId = id;
    }
}