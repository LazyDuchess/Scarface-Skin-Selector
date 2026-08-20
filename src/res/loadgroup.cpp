#include "res/loadgroup.h"

static void WritePaddedString(FILE* file, const std::string& str) {
	int8_t length = static_cast<int8_t>(str.size());
	int8_t pad = (4 - (length % 4)) % 4;
	int8_t fullLength = length + pad;
	fwrite(&fullLength, sizeof(fullLength), 1, file);
	fwrite(str.data(), 1, length, file);
	for (int i = 0; i < pad; i++) {
		fputc(0, file);
	}
}

LoadObject::LoadObject(const std::string& name, const std::string& path)
	: m_Name(name), m_Path(path) {
}

void LoadObject::Write(FILE* file) {
	auto beginPos = ftell(file);
	// Header
	fputc(0x50, file);
	fputc(0x01, file);
	fputc(0x80, file);
	fputc(0x08, file);
	auto sizePos = ftell(file);
	// Size placeholder
	const int32_t placeHolder = 0;
	fwrite(&placeHolder, sizeof(int32_t), 1, file);
	fwrite(&placeHolder, sizeof(int32_t), 1, file);
	WritePaddedString(file, m_Name);
	// Unknown flags?
	fputc(0x06, file);
	fputc(0x00, file);
	fputc(0x00, file);
	fputc(0x01, file);
	fputc(0x00, file);
	fputc(0x00, file);
	fputc(0x00, file);
	WritePaddedString(file, m_Path);
	auto endPos = ftell(file);
	fseek(file, sizePos, SEEK_SET);
	int32_t size = static_cast<int32_t>(endPos - beginPos);
	fwrite(&size, sizeof(int32_t), 1, file);
	fwrite(&size, sizeof(int32_t), 1, file);
	fseek(file, endPos, SEEK_SET);
}

void LoadGroup::Write(FILE* file) {
	auto beginPos = ftell(file);
	//magic
	const unsigned char magic[] = { 'P','3','D', 0xFF, 0x0C, 0x00, 0x00, 0x00 };
	fwrite(magic, 1, sizeof(magic), file);
	auto sizePos = ftell(file);
	// size placeholder
	const int32_t placeHolder = 0;
	fwrite(&placeHolder, sizeof(int32_t), 1, file);
	for (auto& loadObject : m_LoadObjects) {
		loadObject.Write(file);
	}
	auto endPos = ftell(file);
	fseek(file, sizePos, SEEK_SET);
	int32_t size = static_cast<int32_t>(endPos - beginPos);
	fwrite(&size, sizeof(int32_t), 1, file);
	fseek(file, endPos, SEEK_SET);
}