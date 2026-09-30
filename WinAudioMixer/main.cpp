// Cross-platform serial reader for Arduino-style line based output.

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace
{
std::string defaultPort()
{
#ifdef _WIN32
	return "COM3";
#else
	return "/dev/cu.usbmodem1301";
#endif
}

int defaultBaud()
{
	return 9600;
}

bool parseInt(const std::string &text, int &value)
{
	try
	{
		size_t consumed = 0;
		const int parsed = std::stoi(text, &consumed);
		if (consumed != text.size())
			return false;

		value = parsed;
		return true;
	}
	catch (const std::exception &)
	{
		return false;
	}
}

void printUsage(const char *programName)
{
	std::cout << "Usage: " << programName << " [port] [baud]\n";
	std::cout << "Example macOS: " << programName << " /dev/cu.usbmodem1301 9600\n";
	std::cout << "Example Windows: " << programName << " COM3 9600\n";
}

class SerialPort
{
public:
	~SerialPort()
	{
		close();
	}

	bool open(const std::string &portName, int baudRate, std::string &errorMessage)
	{
#ifdef _WIN32
		std::wstring widePort = toWidePortName(portName);
		m_handle = CreateFileW(
			widePort.c_str(),
			GENERIC_READ,
			0,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr);

		if (m_handle == INVALID_HANDLE_VALUE)
		{
			errorMessage = "Could not open serial port.";
			return false;
		}

		DCB dcb{};
		dcb.DCBlength = sizeof(dcb);

		if (!GetCommState(m_handle, &dcb))
		{
			errorMessage = "Could not read serial port settings.";
			close();
			return false;
		}

		dcb.BaudRate = static_cast<DWORD>(baudRate);
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.fBinary = TRUE;
		dcb.fParity = FALSE;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fTXContinueOnXoff = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fErrorChar = FALSE;
		dcb.fNull = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		if (!SetCommState(m_handle, &dcb))
		{
			errorMessage = "Could not configure serial port.";
			close();
			return false;
		}

		COMMTIMEOUTS timeouts{};
		timeouts.ReadIntervalTimeout = 50;
		timeouts.ReadTotalTimeoutConstant = 50;
		timeouts.ReadTotalTimeoutMultiplier = 10;
		timeouts.WriteTotalTimeoutConstant = 50;
		timeouts.WriteTotalTimeoutMultiplier = 10;

		if (!SetCommTimeouts(m_handle, &timeouts))
		{
			errorMessage = "Could not set serial timeouts.";
			close();
			return false;
		}

		PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_TXABORT);
		return true;
#else
		m_fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

		if (m_fd < 0)
		{
			errorMessage = "Could not open serial port.";
			return false;
		}

		termios tty{};
		if (tcgetattr(m_fd, &tty) != 0)
		{
			errorMessage = "Could not read serial port settings.";
			close();
			return false;
		}

		cfmakeraw(&tty);
		tty.c_cflag |= (CLOCAL | CREAD);
		tty.c_cflag &= ~CSTOPB;
		tty.c_cflag &= ~CRTSCTS;
		tty.c_cflag &= ~PARENB;
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS8;

		if (!setSpeed(tty, baudRate))
		{
			errorMessage = "Unsupported baud rate on this platform.";
			close();
			return false;
		}

		tty.c_cc[VMIN] = 0;
		tty.c_cc[VTIME] = 1;

		if (tcsetattr(m_fd, TCSANOW, &tty) != 0)
		{
			errorMessage = "Could not configure serial port.";
			close();
			return false;
		}

		tcflush(m_fd, TCIOFLUSH);
		return true;
#endif
	}

	bool readByte(char &byte)
	{
#ifdef _WIN32
		DWORD bytesRead = 0;
		if (!ReadFile(m_handle, &byte, 1, &bytesRead, nullptr))
			return false;
		return bytesRead == 1;
#else
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(m_fd, &readSet);

		timeval timeout{};
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;

		const int ready = select(m_fd + 1, &readSet, nullptr, nullptr, &timeout);
		if (ready <= 0)
			return false;

		const ssize_t bytesRead = ::read(m_fd, &byte, 1);
		return bytesRead == 1;
#endif
	}

	void close()
	{
#ifdef _WIN32
		if (m_handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_handle);
			m_handle = INVALID_HANDLE_VALUE;
		}
#else
		if (m_fd >= 0)
		{
			::close(m_fd);
			m_fd = -1;
		}
#endif
	}

private:
#ifdef _WIN32
	static std::wstring toWidePortName(const std::string &portName)
	{
		const std::wstring wideName(portName.begin(), portName.end());
		if (wideName.rfind(L"\\\\.\\", 0) == 0)
			return wideName;

		if (wideName.rfind(L"COM", 0) == 0 || wideName.rfind(L"com", 0) == 0)
			return L"\\\\.\\" + wideName;

		return wideName;
	}

	HANDLE m_handle = INVALID_HANDLE_VALUE;
#else
	static bool setSpeed(termios &tty, int baudRate)
	{
		speed_t speed = B0;

		switch (baudRate)
		{
		case 1200: speed = B1200; break;
		case 2400: speed = B2400; break;
		case 4800: speed = B4800; break;
		case 9600: speed = B9600; break;
		case 19200: speed = B19200; break;
		case 38400: speed = B38400; break;
		case 57600: speed = B57600; break;
		case 115200: speed = B115200; break;
		case 230400: speed = B230400; break;
		default: return false;
		}

		return cfsetispeed(&tty, speed) == 0 && cfsetospeed(&tty, speed) == 0;
	}

	int m_fd = -1;
#endif
};

void handleLine(const std::string &line)
{
	if (line.empty())
		return;

	std::cout << "Arduino:";

	size_t start = 0;
	int index = 0;
	while (start <= line.size())
	{
		const size_t separator = line.find('|', start);
		const std::string token = line.substr(start, separator == std::string::npos ? std::string::npos : separator - start);

		if (!token.empty())
		{
			char *end = nullptr;
			const long value = std::strtol(token.c_str(), &end, 10);
			if (end != token.c_str() && *end == '\0')
			{
				std::cout << " [" << index << "]=" << value;
			}
			else
			{
				std::cout << " [" << index << "]=" << token;
			}
		}

		if (separator == std::string::npos)
			break;

		start = separator + 1;
		++index;
	}

	std::cout << std::endl;
}
} // namespace

int main(int argc, char *argv[])
{
	const std::string portName = argc > 1 ? argv[1] : defaultPort();

	int baudRate = defaultBaud();
	if (argc > 2 && !parseInt(argv[2], baudRate))
	{
		std::cerr << "Ungultige Baudrate: " << argv[2] << std::endl;
		printUsage(argv[0]);
		return 1;
	}

	if (argc == 1)
	{
		std::cout << "Kein Port angegeben, verwende Standard: " << portName << std::endl;
	}

	SerialPort serial;
	std::string errorMessage;

	if (!serial.open(portName, baudRate, errorMessage))
	{
		std::cerr << errorMessage << " Port: " << portName << " Baud: " << baudRate << std::endl;
		return 1;
	}

	std::cout << "Lese Serial-Daten von " << portName << " mit " << baudRate << " Baud. Beenden mit Strg+C." << std::endl;

	std::string currentLine;
	char byte = '\0';

	while (true)
	{
		if (!serial.readByte(byte))
			continue;

		if (byte == '\r')
			continue;

		if (byte == '\n')
		{
			handleLine(currentLine);
			currentLine.clear();
			continue;
		}

		currentLine.push_back(byte);
	}

	return 0;
}
