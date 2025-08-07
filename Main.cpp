#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <string_view>

#pragma comment(lib, "ws2_32.lib")

struct BufferReader {
    char* m_array = NULL;
    size_t m_position = 0;
    size_t m_limit = 0;

    size_t remaining() {
        return (m_limit - m_position);
    }

    bool has_remaining() {
        return remaining() > 0;
    }

    size_t position() {
        return m_position;
    }

    void position(size_t pos) {
        if (pos > m_limit) {
            throw std::out_of_range("Position is greater than limit.");
        }

        m_position = pos;
    }

    void get(const char* to, size_t offset, size_t length) {
        if (remaining() < length) {
            throw std::out_of_range("Insufficient data remaining.");
        }

        ::memcpy((void*)(to + offset), m_array + m_position, length);

        m_position += length;
    }

    void get(char& ch) {
        if (!has_remaining()) {
            throw std::out_of_range("Insufficient data remaining.");
        }

        ch = m_array[m_position];

        ++m_position;
    }

    void get(uint8_t& i) {
        char ch;

        get(ch);

        i = (uint8_t)ch;
    }

    void get(uint16_t& i) {
        uint16_t n = 0;

        get((const char*)&n, 0, sizeof(uint16_t));

        i = ntohs(n);
    }

    void get(uint32_t& i) {
        uint32_t n = 0;

        get((const char*)&n, 0, sizeof(uint32_t));

        i = ntohl(n);
    }

    void get(std::string_view& sv, size_t length) {
        if (remaining() < length) {
            throw std::out_of_range("Insufficient data remaining.");
        }

        sv = { m_array + m_position, length };

        m_position += length;
    }
};

size_t base64_length(std::string_view data) {
	size_t len = data.size();
    size_t output_length = 4 * ((len + 2) / 3);

	return output_length;
}

void base64_encode(std::string_view data, char* result) {
	static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t len = data.size();
	size_t padding = (3 - (len % 3)) % 3;
	size_t output_length = 4 * ((len + 2) / 3);

	size_t j = 0;
	for (size_t i = 0; i < len; i += 3) {
		uint32_t octet_a = i < len ? (unsigned char)data[i] : 0;
		uint32_t octet_b = i + 1 < len ? (unsigned char)data[i + 1] : 0;
		uint32_t octet_c = i + 2 < len ? (unsigned char)data[i + 2] : 0;

		uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

		result[j++] = base64_chars[(triple >> 18) & 0x3F];
		result[j++] = base64_chars[(triple >> 12) & 0x3F];
		result[j++] = base64_chars[(triple >> 6) & 0x3F];
		result[j++] = base64_chars[triple & 0x3F];
	}

	for (size_t i = 0; i < padding; ++i) {
		result[output_length - 1 - i] = '=';
	}
}

void copy_URL(std::string_view data, std::string_view mime_type) {
    std::string prefix = "<img src=\"data:";
    
    prefix.append(mime_type);
    prefix.append(";base64,");

	std::string_view suffix = "\" alt=\"Embedded Image\" />";
	size_t base64_size = base64_length(data);
	size_t total_size = prefix.size() + base64_size + suffix.size() + 1; //Add 1 for null terminator

	// Empty the clipboard
	EmptyClipboard();

	// Allocate global memory for the data
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, total_size);

	if (!hGlobal) {
		std::cerr << "Failed to allocate global memory. Error: " << GetLastError() << std::endl;

		return;
	}

	// Lock the global memory and copy the data
	char* pData = static_cast<char*>(GlobalLock(hGlobal));

	if (pData) {
		// Copy the prefix
		std::memcpy(pData, prefix.data(), prefix.size());
        //Encode the image data
        base64_encode(data, pData + prefix.size());
        //Copy the suffix
		std::memcpy(pData + prefix.size() + base64_size, suffix.data(), suffix.size());

		pData[total_size - 1] = '\0'; // Null-terminate the string

        GlobalUnlock(hGlobal);

		// Set the clipboard data
		if (SetClipboardData(CF_TEXT, hGlobal) == NULL) {
			std::cerr << "Failed to set clipboard data. Error: " << GetLastError() << std::endl;
		}
	}
	else {
		std::cerr << "Failed to lock global memory. Error: " << GetLastError() << std::endl;
	}

    GlobalFree(hGlobal);
}

void process_png(char *pData, size_t size) {
    BufferReader reader;

	reader.m_array = pData;
	reader.m_position = 0;
	reader.m_limit = size;

    char list[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

    for (auto ch : list) {
        char c;

        reader.get(c);

        if (c != ch) {
            std::cout << "Invalid PNG found" << std::endl;
            std::cout << "Error: " << (int)c << " != " << (int)ch << std::endl;
            return;
        }
    }

    while (reader.has_remaining()) {
        uint32_t sz;

        reader.get(sz);

        //std::cout << "Chunk size: " << sz << std::endl;

        std::string_view type{};

        reader.get(type, 4);

        //std::cout << "Chunk type: " << type << std::endl;

        //Skip past the data
        reader.position(reader.position() + sz);

		//Read the CRC
		uint32_t crc;

		reader.get(crc);

        if (type == "IEND") {
            //std::cout << "End of PNG. Size: " << reader.position() << std::endl;

            break;
        }
    }

	std::string_view data(pData, reader.position());

	copy_URL(data, "image/png");

    /*
    FILE* f = NULL;

	if (fopen_s(&f, "output.png", "wb") == 0) {
		fwrite(pData, 1, reader.position(), f);
		fclose(f);
		std::cout << "PNG data written to output.png" << std::endl;
	}
    else {
        std::cerr << "Failed to write PNG data to file." << std::endl;
    }
    */
}

int main() {
    // Open the clipboard
    if (!OpenClipboard(NULL)) {
        std::cerr << "Failed to open clipboard. Error: " << GetLastError() << std::endl;
        return 1;
    }

    //Register "PNG" format
	UINT pngFormat = RegisterClipboardFormatA("PNG");

	//Get the text data from the clipboard
    HANDLE hData = NULL;

	if ((hData = GetClipboardData(CF_TEXT)) != NULL) {
		char* pData = static_cast<char*>(GlobalLock(hData));

		if (pData) {
			std::string_view data(pData);

			//Check if the data starts with the SVG header
            if (data.starts_with("<?xml") || data.starts_with("<svg")) {
                copy_URL(data, "image/svg+xml");
			}
			else {
				std::cout << "Clipboard does not contain SVG data." << std::endl;
			}

			GlobalUnlock(hData);
		}
		else {
			std::cerr << "Failed to lock clipboard data. Error: " << GetLastError() << std::endl;
		}
	}
	else if ((hData = GetClipboardData(pngFormat)) != NULL) {
        char* pData = static_cast<char*>(GlobalLock(hData));

        if (pData) {
            //Get size of data
            size_t size = GlobalSize(hData);

            process_png(pData, size);

            GlobalUnlock(hData);
        }
        else {
            std::cerr << "Failed to lock clipboard data. Error: " << GetLastError() << std::endl;
        }
	}
	else {
		std::cout << "Did not find SVG or PNG in clipboard." << std::endl;
	}

    // Close the clipboard
    CloseClipboard();

    return 0;
}