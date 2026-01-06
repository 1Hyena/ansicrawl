// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////


USER *user_create() {
    USER *user = mem_new_user();

    if (!user) {
        return nullptr;
    }

    if ((user->io.incoming.clip = clip_create_byte_array()) == nullptr
    || !(user->io.outgoing.clip = clip_create_byte_array())) {
        user_destroy(user);

        return nullptr;
    }

    return user;
}

void user_destroy(USER *user) {
    if (!user) {
        return;
    }

    clip_destroy(user->io.incoming.clip);
    clip_destroy(user->io.outgoing.clip);

    mem_free_user(user);
}
