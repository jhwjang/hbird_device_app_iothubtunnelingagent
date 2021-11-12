#pragma once

#define HWANJANG_LOG(fmt, ...) { printf("[hwanjang] [%s : %d] %s -> "fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); }

