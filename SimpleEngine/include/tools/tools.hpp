#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace SE
{

inline void SwitchOnBitFlag(unsigned int& flags, unsigned int bit) { flags |= bit; }

inline void SwitchOffBitFlag(unsigned int& flags, unsigned int bit) { flags &= (~bit); }

inline bool CheckBitFlag(unsigned int flags, unsigned int bit) { return (flags & bit) == bit; }

inline bool CheckBitFlagOverlap(unsigned int flag0, unsigned int flag1) { return (flag0 & flag1) != 0; }

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
inline uint32_t HashCombine(uint32_t lhs, uint32_t rhs)
{
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
}

inline glm::vec3 to_vec3(const glm::vec2& vec) { return glm::vec3(vec.x, vec.y, 0.0f); }

inline glm::vec3 to_vec3(std::vector<double> array) { return glm::vec3((float)array[0], (float)array[1], (float)array[2]); }

inline glm::quat to_quat(std::vector<double> array)
{
    return glm::quat((float)array[3], (float)array[0], (float)array[1], (float)array[2]);
}

inline glm::vec4 to_vec4(std::vector<double> array)
{
    return glm::vec4((float)array[0], (float)array[1], (float)array[2], (float)array[3]);
}

std::string StringReplace(const std::string& subject, const std::string& search, const std::string& replace);

bool StringEndsWith(const std::string& subject, const std::string& suffix);

bool StringStartsWith(const std::string& subject, const std::string& prefix);

std::vector<std::string> SplitString(const std::string& input, const std::string& delim);

float GetRandomNumber(float min, float max, int decimals = 3);

glm::vec3 HSVtoRGB(glm::vec3 hsv);

glm::vec3 RandomNiceColor(int i, float s = 0.5f, float v = 0.95f);

}