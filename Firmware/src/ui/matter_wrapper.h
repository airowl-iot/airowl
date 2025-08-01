#ifndef MATTER_WRAPPER_H
#define MATTER_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#ifdef CONFIG_ESP_MATTER_ENABLE

void initMatter();           // Call once in setup
void matter_loop();          // Call repeatedly in loop
bool is_matter_commissioned(); // Optional helper

#endif

#ifdef __cplusplus
}
#endif

#endif

