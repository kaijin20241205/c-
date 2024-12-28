#include "./../include/MyMemoryHook.h"

MemRecord *head = nullptr;

void add_record(void *address, size_t size) {
    MemRecord *new_record = (MemRecord *)::malloc(sizeof(MemRecord));
    new_record->address = address;
    new_record->size = size;
    new_record->next = head;
    head = new_record;
    printf("1\n");
}

void remove_record(void *address) {
    MemRecord **current = &head;
    while (*current) {
        if ((*current)->address == address) {
            MemRecord *to_free = *current;
            *current = (*current)->next;
            printf("2\n");
            ::free(to_free);
            return;
        }
        current = &(*current)->next;
    }
}

void check_leaks() {
    MemRecord *current = head;
    if (!current) {
        printf("No memory leaks detected.\n");
        return;
    }

    printf("Memory leaks detected:\n");
    while (current) {
        printf("Leak: Address %p, Size %zu bytes\n", current->address, current->size);
        current = current->next;
    }
}

void *my_malloc(size_t size) {
    void *ptr = ::malloc(size);
    if (ptr) {
        add_record(ptr, size);
    }
    return ptr;
}

void my_free(void *ptr) {
    if (ptr) {
        remove_record(ptr);
        ::free(ptr);
    }
}
