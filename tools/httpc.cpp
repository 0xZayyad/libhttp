#include "../lib/libhttp.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <map>

using namespace nsh;

struct CmdOptions
{
	std::string url;
	std::string method{"GET"};
	std::vector<std::string> headers;
	std::string userAgent;
	std::string proxyUrl;
	std::string data;
	std::string dataFile;
	std::string contentType;
	std::string outputFile;
	std::string dumpHeaderFile;
	bool include{false};
	bool verbose{false};
	bool headOnly{false};
	bool followRedirects{false};
	int maxRedirects{10};
	int responseTimeout{0};
	int httpVersion{0}; // 0=default, 10=1.0, 11=1.1, 20=2
	int tlsVersion{0};	// 0=default, 10,11,12,13
	bool failOnHttpError{false};
};

static void printUsage()
{
	std::cout << "httpc - HTTP client using libhttp\n\n"
						<< "Usage: httpc [options] <url>\n\n"
						<< "Options:\n"
						<< "  -X, --request <METHOD>       Specify request method (GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS, TRACE)\n"
						<< "  -H, --header <LINE>          Add request header (e.g., 'Header: value')\n"
						<< "  -A, --user-agent <UA>        Set User-Agent\n"
						<< "  -d, --data <DATA>            HTTP request body (string)\n"
						<< "      --data-file <FILE>       HTTP request body from file\n"
						<< "      --content-type <TYPE>    Set Content-Type header\n"
						<< "  -x, --proxy <URL>            Use HTTP proxy (e.g., http://127.0.0.1:8080)\n"
						<< "  -o, --output <FILE>          Write response body to file\n"
						<< "  -D, --dump-header <FILE>     Write response headers to file\n"
						<< "  -i, --include                Include response headers in output\n"
						<< "  -v, --verbose                Enable verbose logging\n"
						<< "  -I, --head                   Fetch headers only (HEAD)\n"
						<< "  -L, --location               Follow redirects\n"
						<< "      --max-redirs <N>         Maximum number of redirects (default 10)\n"
						<< "      --max-time <SEC>         Response timeout seconds\n"
						<< "      --http1.0|--http1.1|--http2  Force HTTP version\n"
						<< "      --tlsv1.0|--tlsv1.1|--tlsv1.2|--tlsv1.3  Force TLS version\n"
						<< "      --fail                    Exit non-zero on HTTP >= 400\n"
						<< std::endl;
}

static bool startsWith(const char *s, const char *p) { return std::strncmp(s, p, std::strlen(p)) == 0; }

static bool parseArgs(int argc, char **argv, CmdOptions &opt)
{
	if (argc < 2)
	{
		printUsage();
		return false;
	}
	for (int i = 1; i < argc; ++i)
	{
		std::string a = argv[i];
		if (a == "-h" || a == "--help")
		{
			printUsage();
			return false;
		}
		else if (a == "-X" || a == "--request")
		{
			if (++i >= argc)
				return false;
			opt.method = argv[i];
		}
		else if (a == "-H" || a == "--header")
		{
			if (++i >= argc)
				return false;
			opt.headers.emplace_back(argv[i]);
		}
		else if (a == "-A" || a == "--user-agent")
		{
			if (++i >= argc)
				return false;
			opt.userAgent = argv[i];
		}
		else if (a == "-d" || a == "--data")
		{
			if (++i >= argc)
				return false;
			opt.data = argv[i];
		}
		else if (a == "--data-file")
		{
			if (++i >= argc)
				return false;
			opt.dataFile = argv[i];
		}
		else if (a == "--content-type")
		{
			if (++i >= argc)
				return false;
			opt.contentType = argv[i];
		}
		else if (a == "-x" || a == "--proxy")
		{
			if (++i >= argc)
				return false;
			opt.proxyUrl = argv[i];
		}
		else if (a == "-o" || a == "--output")
		{
			if (++i >= argc)
				return false;
			opt.outputFile = argv[i];
		}
		else if (a == "-D" || a == "--dump-header")
		{
			if (++i >= argc)
				return false;
			opt.dumpHeaderFile = argv[i];
		}
		else if (a == "-i" || a == "--include")
		{
			opt.include = true;
		}
		else if (a == "-v" || a == "--verbose")
		{
			opt.verbose = true;
		}
		else if (a == "-I" || a == "--head")
		{
			opt.headOnly = true;
			opt.method = "HEAD";
		}
		else if (a == "-L" || a == "--location")
		{
			opt.followRedirects = true;
		}
		else if (a == "--max-redirs")
		{
			if (++i >= argc)
				return false;
			opt.maxRedirects = std::atoi(argv[i]);
		}
		else if (a == "--max-time")
		{
			if (++i >= argc)
				return false;
			opt.responseTimeout = std::atoi(argv[i]);
		}
		else if (a == "--http1.0")
		{
			opt.httpVersion = 10;
		}
		else if (a == "--http1.1")
		{
			opt.httpVersion = 11;
		}
		else if (a == "--http2")
		{
			opt.httpVersion = 20;
		}
		else if (a == "--tlsv1.0")
		{
			opt.tlsVersion = 10;
		}
		else if (a == "--tlsv1.1")
		{
			opt.tlsVersion = 11;
		}
		else if (a == "--tlsv1.2")
		{
			opt.tlsVersion = 12;
		}
		else if (a == "--tlsv1.3")
		{
			opt.tlsVersion = 13;
		}
		else if (a == "--fail")
		{
			opt.failOnHttpError = true;
		}
		else if (!a.empty() && a[0] != '-')
		{
			opt.url = a;
		}
		else
		{
			std::cerr << "Unknown option: " << a << "\n";
			return false;
		}
	}
	if (opt.url.empty())
	{
		std::cerr << "Missing URL\n";
		return false;
	}
	if (opt.method.empty())
		opt.method = "GET";
	if (!opt.data.empty() && opt.method == "GET")
		opt.method = "POST"; // curl-like behavior
	return true;
}

static int methodToEnum(const std::string &m)
{
	static std::map<std::string, int> map = {
			{"GET", HTTP_GET}, {"POST", HTTP_POST}, {"PUT", HTTP_PUT}, {"PATCH", HTTP_PATCH}, {"HEAD", HTTP_HEAD}, {"OPTIONS", HTTP_OPTIONS}, {"DELETE", HTTP_DELETE}, {"TRACE", HTTP_TRACE}};
	auto it = map.find(m);
	if (it == map.end())
		return HTTP_GET;
	return it->second;
}

static void applyCommonOptions(HTTPSession &s, const CmdOptions &opt)
{
	if (opt.verbose)
		s.setOption(HTTP_OPTIONS_VERBOSITY, (long int)HTTP_VERBOSITY_ENABLE);
	s.setOption(HTTP_OPTIONS_URL, opt.url.c_str());
	if (!opt.userAgent.empty())
		s.setOption(HTTP_OPTIONS_USER_AGENT, opt.userAgent.c_str());
	if (opt.httpVersion == 10)
		s.setOption(HTTP_OPTIONS_HTTP_VERSION, (long int)HTTP_1_0);
	else if (opt.httpVersion == 11)
		s.setOption(HTTP_OPTIONS_HTTP_VERSION, (long int)HTTP_1_1);
	else if (opt.httpVersion == 20)
		s.setOption(HTTP_OPTIONS_HTTP_VERSION, (long int)HTTP_2);
	if (opt.tlsVersion == 10)
		s.setOption(HTTP_OPTIONS_TLS_VERSION, (long int)HTTP_TLS_1_0);
	else if (opt.tlsVersion == 11)
		s.setOption(HTTP_OPTIONS_TLS_VERSION, (long int)HTTP_TLS_1_1);
	else if (opt.tlsVersion == 12)
		s.setOption(HTTP_OPTIONS_TLS_VERSION, (long int)HTTP_TLS_1_2);
	else if (opt.tlsVersion == 13)
		s.setOption(HTTP_OPTIONS_TLS_VERSION, (long int)HTTP_TLS_1_3);
	if (opt.responseTimeout > 0)
		s.setOption(HTTP_OPTIONS_RESPONSE_TIMEOUT, (long int)opt.responseTimeout);
	if (opt.followRedirects)
	{
		s.setOption(HTTP_OPTIONS_REDIRECTS, (long int)HTTP_REDIRECTS_ALLOW);
		if (opt.maxRedirects > 0)
			s.setOption(HTTP_OPTIONS_MAX_REDIRECT, (long int)opt.maxRedirects);
	}
	// Headers
	if (!opt.headers.empty() || !opt.contentType.empty())
	{
		std::string all;
		for (auto &h : opt.headers)
		{
			all += h;
			if (!h.empty() && h.back() != '\n')
				all += "\r\n";
		}
		if (!opt.contentType.empty())
		{
			all += "Content-Type: ";
			all += opt.contentType;
			// all += "\r\n";
		}
		if (!all.empty())
			s.setOption(HTTP_OPTIONS_HEADERS_INCLUDE, all.c_str());
	}
	// Body
	int m = methodToEnum(opt.method);
	if (!opt.data.empty() || !opt.dataFile.empty())
	{
		if (m == HTTP_POST)
		{
			if (!opt.data.empty())
				s.setOption(HTTP_OPTIONS_POST_BODY, opt.data.c_str());
			else
				s.setOption(HTTP_OPTIONS_POST_BODY_FILE, opt.dataFile.c_str());
		}
		else if (m == HTTP_PUT)
		{
			if (!opt.data.empty())
				s.setOption(HTTP_OPTIONS_PUT_BODY, opt.data.c_str());
			else
				s.setOption(HTTP_OPTIONS_PUT_BODY_FILE, opt.dataFile.c_str());
		}
		else if (m == HTTP_PATCH)
		{
			if (!opt.data.empty())
				s.setOption(HTTP_OPTIONS_PATCH_BODY, opt.data.c_str());
			else
				s.setOption(HTTP_OPTIONS_PATCH_BODY_FILE, opt.dataFile.c_str());
		}
	}
	// Set method
	s.setOption(HTTP_OPTIONS_REQUEST_METHOD, (long int)m);
}

int main(int argc, char **argv)
{
	CmdOptions opt;
	if (!parseArgs(argc, argv, opt))
		return 2;

	FILE *out = nullptr;
	FILE *hdr = nullptr;
	if (!opt.outputFile.empty())
	{
		out = std::fopen(opt.outputFile.c_str(), "wb");
		if (!out)
		{
			std::perror("open output");
			return 2;
		}
	}
	if (!opt.dumpHeaderFile.empty())
	{
		hdr = std::fopen(opt.dumpHeaderFile.c_str(), "wb");
		if (!hdr)
		{
			std::perror("open header file");
			if (out)
				std::fclose(out);
			return 2;
		}
	}

	try
	{
		if (!opt.proxyUrl.empty())
		{
			HTTPProxySession s{opt.proxyUrl.c_str()};
			s.setOption(HTTP_OPTIONS_URL, opt.url.c_str());
			applyCommonOptions(s, opt);
			// Establish proxy tunnel then perform through proxy
			s.connect();
			s.proxySendRequest();
			if (opt.method == "GET")
				s.httpGet();
			else if (opt.method == "POST")
				s.httpPost();
			else if (opt.method == "PUT")
				s.httpPut();
			else if (opt.method == "PATCH")
				s.httpPatch();
			else if (opt.method == "HEAD")
				s.httpHead();
			else if (opt.method == "OPTIONS")
				s.httpOptions();
			else if (opt.method == "TRACE")
				s.httpTrace();
			else if (opt.method == "DELETE")
				s.httpDelete();
			else
				s.performRequest();

			int code = s.getStatusCode();
			if (hdr)
				s.writeResponseHeaders(hdr);
			if (opt.include)
				s.writeResponseHeadersFriendly(stdout);
			if (out)
				s.writeResponseBody(out);
			else
				s.writeResponseBodyFriendly(stdout);
			if (out)
				std::fclose(out);
			if (hdr)
				std::fclose(hdr);
			if (opt.failOnHttpError && code >= 400)
				return 22; // curl-like
		}
		else
		{
			HTTPSession s{opt.url.c_str()};
			applyCommonOptions(s, opt);
			s.connect();
			if (opt.method == "GET")
				s.httpGet();
			else if (opt.method == "POST")
				s.httpPost();
			else if (opt.method == "PUT")
				s.httpPut();
			else if (opt.method == "PATCH")
				s.httpPatch();
			else if (opt.method == "HEAD")
				s.httpHead();
			else if (opt.method == "OPTIONS")
				s.httpOptions();
			else if (opt.method == "TRACE")
				s.httpTrace();
			else if (opt.method == "DELETE")
				s.httpDelete();
			else
				s.performRequest();

			int code = s.getStatusCode();
			if (hdr)
				s.writeResponseHeaders(hdr);
			if (opt.include)
				s.writeResponseHeadersFriendly(stdout);
			if (out)
				s.writeResponseBody(out);
			else
				s.writeResponseBodyFriendly(stdout);
			if (out)
				std::fclose(out);
			if (hdr)
				std::fclose(hdr);
			if (opt.failOnHttpError && code >= 400)
				return 22;
		}
	}
	catch (HTTPException &e)
	{
		std::cerr << "Error: " << e.getError() << std::endl;
		if (out)
			std::fclose(out);
		if (hdr)
			std::fclose(hdr);
		return 1;
	}
	return 0;
}