// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    mod_soap.cpp - this is a plugin soap for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/xml/soap/soap_object.h>
#include <ulib/xml/soap/soap_parser.h>
#include <ulib/net/server/plugin/mod_soap.h>

U_CREAT_FUNC(server_plugin_soap, USoapPlugIn)

USOAPParser* USoapPlugIn::soap_parser;

USoapPlugIn::~USoapPlugIn()
{
   U_TRACE_DTOR(0, USoapPlugIn)

   if (soap_parser)
      {
      U_DELETE(soap_parser)
      U_DELETE(URPCMethod::encoder)
      U_DELETE(URPCObject::dispatcher)
      }
}

// Server-wide hooks

int USoapPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "USoapPlugIn::handlerConfig(%p)", &cfg)

   // Perform registration of server SOAP method

   U_NEW(USOAPParser, soap_parser, USOAPParser);

   USOAPObject::loadGenericMethod(&cfg);

   U_RETURN(U_PLUGIN_HANDLER_PROCESSED);
}

int USoapPlugIn::handlerInit()
{
   U_TRACE_NO_PARAM(0, "USoapPlugIn::handlerInit()")

   // NB: soap is NOT a static page, so to avoid stat() syscall we use alias mechanism...

   if (soap_parser)
      {
#  ifndef U_ALIAS
      U_SRV_LOG("WARNING: Sorry, I can't run soap plugin because alias URI support is missing, please recompile ULib...");
#  else
      if (UHTTP::valias == U_NULLPTR) U_NEW(UVector<UString>, UHTTP::valias, UVector<UString>(2U))

      UHTTP::valias->push_back(*UString::str_soap);
      UHTTP::valias->push_back(*UString::str_nostat);

      U_RETURN(U_PLUGIN_HANDLER_OK);
#  endif
      }

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// Connection-wide hooks

int USoapPlugIn::handlerRequest()
{
   U_TRACE_NO_PARAM(0, "USoapPlugIn::handlerRequest()")

   if (UHTTP::isSOAPRequest())
      {
      if (soap_parser == U_NULLPTR) UHTTP::setInternalError();
      else
         {
         // process the SOAP message -- should be the contents of the message from "<SOAP:" to the end of the string

         bool bSendingFault;

         UString body   = soap_parser->processMessage(*UClientImage_Base::body, *URPCObject::dispatcher, bSendingFault),
                 method = soap_parser->getMethodName();

         U_SRV_LOG_WITH_ADDR("method %V process %s for", method.rep, (bSendingFault ? "failed" : "passed"));

#     ifdef DEBUG
         U_FILE_WRITE_TO_TMP(body, "soap.res");
#     endif

         U_http_info.nResponseCode = HTTP_OK;

         UHTTP::setResponse(*UString::str_ctype_soap, &body);
         }

      U_RETURN(U_PLUGIN_HANDLER_PROCESSED);
      }

   U_RETURN(U_PLUGIN_HANDLER_OK);
}

// DEBUG

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
const char* USoapPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "soap_parser (USOAPParser " << (void*)soap_parser << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return U_NULLPTR;
}
#endif
