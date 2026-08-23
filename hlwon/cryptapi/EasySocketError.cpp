// EasySocketError

#include "EasySocketError.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ES_ErrorTypeToString (0x40D0D0)
std::string ES_ErrorTypeToString( int error )
{
	switch ( error )
	{
	case 0: return "ES_NO_ERROR";
	case 1: return "ES_ERROR_NO_TYPE";
	case 2: return "ES_INVALID_ADDR";
	case 3: return "ES_INVALID_SOCKET";
	case 4: return "ES_INCOMPLETE_SEND";
	case 5: return "ES_INCOMPLETE_RECV";
	case 6: return "ES_DISCONNECTED";
	case 7: return "ES_TMSG_TOO_LARGE";
	case 8: return "ES_WRONG_TSERVICE";
	case 9: return "ES_WRONG_TMESSAGE";
	case 10: return "ES_INVALID_TMSG";
	case 11: return "ES_PARTIAL_SENDTO";
	case 12: return "ES_TIMED_OUT";
	case 13: return "ES_ERROR_STREAM_NOT_ALLOWED";
	case 14: return "ES_SHUTDOWN";
	case 10004: return "ES_WSAEINTR";
	case 10009: return "ES_WSAEBADF";
	case 10013: return "ES_WSAEACCES";
	case 10014: return "ES_WSAEFAULT";
	case 10022: return "ES_WSAEINVAL";
	case 10024: return "ES_WSAEMFILE";
	case 10035: return "ES_WSAEWOULDBLOCK";
	case 10036: return "ES_WSAEINPROGRESS";
	case 10037: return "ES_WSAEALREADY";
	case 10038: return "ES_WSAENOTSOCK";
	case 10039: return "ES_WSAEDESTADDRREQ";
	case 10040: return "ES_WSAEMSGSIZE";
	case 10041: return "ES_WSAEPROTOTYPE";
	case 10042: return "ES_WSAENOPROTOOPT";
	case 10043: return "ES_WSAEPROTONOSUPPORT";
	case 10044: return "ES_WSAESOCKTNOSUPPORT";
	case 10045: return "ES_WSAEOPNOTSUPP";
	case 10046: return "ES_WSAEPFNOSUPPORT";
	case 10047: return "ES_WSAEAFNOSUPPORT";
	case 10048: return "ES_WSAEADDRINUSE";
	case 10049: return "ES_WSAEADDRNOTAVAIL";
	case 10050: return "ES_WSAENETDOWN";
	case 10051: return "ES_WSAENETUNREACH";
	case 10052: return "ES_WSAENETRESET";
	case 10053: return "ES_WSAECONNABORTED";
	case 10054: return "ES_WSAECONNRESET";
	case 10055: return "ES_WSAENOBUFS";
	case 10056: return "ES_WSAEISCONN";
	case 10057: return "ES_WSAENOTCONN";
	case 10058: return "ES_WSAESHUTDOWN";
	case 10059: return "ES_WSAETOOMANYREFS";
	case 10060: return "ES_WSAETIMEDOUT";
	case 10061: return "ES_WSAECONNREFUSED";
	case 10062: return "ES_WSAELOOP";
	case 10063: return "ES_WSAENAMETOOLONG";
	case 10064: return "ES_WSAEHOSTDOWN";
	case 10065: return "ES_WSAEHOSTUNREACH";
	case 10066: return "ES_WSAENOTEMPTY";
	case 10067: return "ES_WSAEPROCLIM";
	case 10068: return "ES_WSAEUSERS";
	case 10069: return "ES_WSAEDQUOT";
	case 10070: return "ES_WSAESTALE";
	case 10071: return "ES_WSAEREMOTE";
	case 10091: return "ES_WSASYSNOTREADY";
	case 10092: return "ES_WSAVERNOTSUPPORTED";
	case 10093: return "ES_WSANOTINITIALISED";
	case 10101: return "ES_WSAEDISCON";
	default: return "";
	}
}
