#pragma once

// Generated by xmake from config/Project.hpp.in

#include <semver.hpp>

namespace App::Project
{
constexpr auto Name = "ArchiveXL";
constexpr auto Author = "psiberx";

constexpr auto NameW = L"ArchiveXL";
constexpr auto AuthorW = L"psiberx";

constexpr auto Version = semver::from_string_noexcept("1.1.5").value();
}