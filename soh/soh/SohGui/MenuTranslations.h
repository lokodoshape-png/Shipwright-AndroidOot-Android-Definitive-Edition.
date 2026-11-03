#pragma once

#include <string>

// Returns the pt-BR text for an English menu string, or nullptr when there is none.
const std::string* MenuTranslations_Find(const std::string& english);
