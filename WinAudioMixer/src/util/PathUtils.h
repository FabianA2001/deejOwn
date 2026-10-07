#pragma once

#include <filesystem>

namespace winaudiomixer {

	// Liefert den Ordner, in dem die laufende .exe (bzw. das Binary) liegt.
	// Bei einem Fehler wird das aktuelle Arbeitsverzeichnis zurückgegeben.
	std::filesystem::path getExecutableDir();

} // namespace winaudiomixer