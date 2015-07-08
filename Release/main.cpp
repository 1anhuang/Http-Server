#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<iostream>
#include <process.h>
#include <sys/stat.h>
#include <time.h>
#include<winsock2.h>

#include"base64.h" ;

using namespace std;
#pragma comment(lib, "ws2_32.lib")

const int BUF_MAX = 1024*32 ;
const int gNumOfType = 7 ;
const int gNumOfStatus = 12 ;
char gWebPath[] = "WebTest" ;
char *gContentType[gNumOfType][2] = { {".html","text/html"}, {".png", "image/png"}, {".jpeg","image/jpeg"}, {".mp3","audio/mpeg"}, {".exe","application/octet-stream" }, {".ico","image/x-icon"}, {".css","text/css"} } ;
char *gUnKnowType = "application/x-unknown" ;


struct SocketInfo{
  SOCKET accept ;
  sockaddr_in from ;
}; // SocketImf

struct StatusCode{
  int statuscode;
  char *statusMsg;
} ; // StatusCode

StatusCode gStatus[gNumOfStatus] = { {200, "HTTP/1.1 200 OK\r\n"}, {201, "HTTP/1.1 201 Created\r\n"}, {204, "HTTP/1.1 204 No Content\r\n"}, {206, "HTTP/1.1 206 Partial content\r\n"},
                                   {304, "HTTP/1.1 304 Not Modified\r\n"}, {301,"HTTP/1.1 301 Moved Permanently\r\n"}, {400, "HTTP/1.1 400 Bad Request\r\n" }, {404, "HTTP/1.1 404 Not Found\r\n" },
                                   {412, "HTTP/1.1 412 Precondition Failed\r\n" }, {501, "HTTP/1.1 501 Not Implemented\r\n"}, {505, "HTTP/1.1 505 HTTP Version Not Supported\r\n" },
                                   {500, "HTTP/1.1 500 Internal Server Error\r\n" } } ;


void ThreadConnect( void *sockInfo ) ;

int main() {

  // socket ��l��
  WSADATA wsData ;
  if ( WSAStartup( MAKEWORD( 2, 2 ), &wsData ) != NOERROR ) {
    printf( "Error: Socket��l��\n" ) ;
    return -1 ;
  }  //if

  SOCKET m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (  m_socket == INVALID_SOCKET ) {
    printf( "Error open socket �s�u���~!\n" ) ;
    WSACleanup();
    return -1 ;
  } // if

  sockaddr_in service ;
  service.sin_family = AF_INET ;
  service.sin_addr.s_addr = INADDR_ANY ;
  service.sin_port = htons( 80 ) ;
  
  if ( bind( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
    printf( "Error: bind\n" ) ;
    closesocket( m_socket ) ;
    return -1 ;
  } // if

  if ( listen( m_socket, 100 ) == SOCKET_ERROR ) {  // �]�w�i�s�u�W��
    printf( "ERROR: listen\n" ) ;
    closesocket( m_socket ) ;
    return -1 ;
  } // if


  printf( "waiting for connection...\n" ) ;
  while ( true ) {
    sockaddr_in from ;
    int fromlen = sizeof( from ) ;
    SOCKET actSocket = accept( m_socket, (struct sockaddr*) &from, &fromlen ) ;
    if ( actSocket == SOCKET_ERROR ) {
      printf( "\nError: accept!\n" ) ;
      closesocket( m_socket ) ;
      return -1 ;
    } // if

    SocketInfo *sockInfo = new SocketInfo() ;
    sockInfo->accept = actSocket ;
    sockInfo->from = from ;

    _beginthread( ThreadConnect,0 ,sockInfo ) ;
    //delete temp ;
  } // while
  
  return 0 ;
} // main

char* ContentType( char* file_name ) {
  for ( int i = 0 ; i < gNumOfType ; i++ ) {
    if ( strcmp( file_name, gContentType[i][0] ) == 0 )
      return gContentType[i][1] ;
  } // for

  return gUnKnowType ;
} // ContentType

void SendHeader( SOCKET* socket, int statuscode, char* header ) {
  char head_status[4096] = "" ;

  // ****���A�P�_******
  for ( int i = 0 ; i < gNumOfStatus ; i++ ) {
    if ( statuscode == gStatus[i].statuscode )
      strcpy( head_status, gStatus[i].statusMsg ) ;
  } // for

  strcat( head_status, "Sever: 10027210\r\n" ) ;
  strcat( head_status, header ) ;
  strcat( head_status, "Connection: close\r\n" ) ; // set Connection
  strcat( head_status, "Set-Cookie: testCookie=\"123\"\r\n" ) ; // set cookie
  printf( "\n[header]:\n%s\n", head_status ) ;
  send( *socket, head_status, strlen( head_status), 0 ) ;
  send( *socket, "\r\n", strlen( "\r\n" ), 0 ) ;    // header����
} // SendHeader

void GetBody( SOCKET *socket, char *body, int bodylen ) {
  char buf[4096] = "" ;
  int recvlen = 0 ;
  for ( int i = 1 ; i != 0 && i != SOCKET_ERROR && recvlen < bodylen ; ) {
    i = recv( *socket, buf, sizeof(buf), 0 ) ;
    recvlen = recvlen + i ;
    strcat( body, buf ) ;
  } // for

} // GetBody

void SendContent( FILE* fp, SOCKET *socket, int range ) {
  int readbyte = 0 ;
  int sendbyte = 0 ;
  char buf[BUF_MAX] = "" ;
  printf( "Sending...\n" ) ;
  for ( int i = 0 ;  !feof( fp ) && sendbyte < range  ; ) {
    i = fread( &buf, sizeof(char), BUF_MAX, fp ) ;
    if ( send( *socket, buf, i, 0 ) == SOCKET_ERROR ) {
      printf( "ERROR: send\n" ) ;
      break ;
    } // if
 
  sendbyte = sendbyte + i ;
  //printf( "%s", buf ) ;
  } // for

} // SendContent

void ThreadConnect( void *sockInfo ) {
  SOCKET m_socket = ( (SocketInfo*)sockInfo )->accept ;
  sockaddr_in from = ( (SocketInfo*)sockInfo )->from ;

  printf( "Connection Succeed!\nclient: adress %s port %d\n\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port) ) ;

  char If_Modified_Since[200] = "" ;  // If-Modified-Since
  char If_None_Match[200] = "" ;      // If-None-Match
  char If_Match[200] = "" ;           // If-Match
  char If_Range[200] = "" ;           // If-Range
  char If_Unmodified_Since[200] = ""; // If-Unmodified-Since
  char Etag[200] = "" ;               // Etag
  char Expect[100] = "" ;             // �P�_���L 100continue
  char request[1024] = "" ;
  char req_status[1024] = "" ;  // �x�sstatus line
  char req_method[20] = "" ;     // �x�s method
  char req_pathandpara[1024] = "" ; // �x�s���|�M�Ѽ�
  char req_path[512] = "" ;     // �n�D�귽�����|
  char req_para[512] = "" ;     // �x�s�Ѽ�
  char filePath[512] = "";      // �}���ɮת����|
  char req_ver[10] = "" ;        // http ����
  char host[50] = "" ;          // �Τ�W
  char connection[50] = "" ;    // Connection
  char header[1024] = "" ;      // �^�Ǫ�header
  char *file_name = "" ;        // �ɮצW
  char *file_extension = "" ;   // ���ɦW
  char filelen[50] = "" ;       // �^�Ǫ� Content-length
  char contenttype[50] = "" ;   // �^�Ǫ� Content-type
  char range[100] = "" ;        // Range
  char time[100] = "" ;         // �^���ɮת��̫�ק�ɶ�
  char resp_lasttime[100] = "" ;// Last-Modified
  char resp_range[100] = "" ;   // �^���ɮת�����
  int range_start = 0 ;         // range�n�D���_�l��m
  int range_end = 0 ;           // ������m
  int recbyte = 0 ;
  int statuscode = 200 ;        // �w�]200 OK
  int bodylen = 0 ;             // �s�����Ǩ�body������
  unsigned long filebyte = 0 ;  // �^�Ǫ��ɮפj�p
  FILE* fp = NULL ;



  // *******get status line*********
  for( int i = 0 ; req_status[recbyte-1] != '\n' ; recbyte = recbyte + i ) {
    i = recv( (SOCKET)m_socket, &req_status[recbyte], 1, 0 ) ;
    if ( i == 0 || i == SOCKET_ERROR ) {  // �p�G���Ÿ�ƴN����
      closesocket( m_socket ) ;
      delete sockInfo ;
      return ;
    } // if
  } // for

  // �ˬdstatusline���e�O�_���`(���׹L�u)
  if ( strlen(req_status ) < strlen( "GET / http/1.1" ) ) {
    closesocket( m_socket ) ;
    delete sockInfo ;
    return ;
  } // if
 
  printf( "status line: %s\n", req_status ) ;
  sscanf( req_status, "%s %s %s", req_method, req_pathandpara, req_ver ) ;
  printf( "method: %s\n", req_method ) ;
  sscanf( req_pathandpara, "%[^?]?%s", req_path, req_para ) ;
  
  if ( strcmp( req_path, "/" ) == 0 ) {
    sprintf( filePath, "%s/index.html", gWebPath ) ;
    printf( "File Path: %s\n\n", filePath ) ;
  } // if
  else {
    sprintf( filePath, "%s%s", gWebPath, req_path ) ;
  } // else

  printf( "start receiving...\n\n" ) ;

  // ***********get header************
  char name[50] = "" ;
  char value[500] = "" ;

  for( int i = 1 ; i != 0 && i != SOCKET_ERROR ; ) {
    char buf[4096] = "" ;
    int datalen = 0 ;
    for( ; buf[datalen-1] != '\n' ; datalen = datalen + i ) {
      i = recv( m_socket, &buf[datalen], 1, 0 ) ;
    } // for

    sscanf( buf, "%[^:]: %[^\r\n]", name, value ) ;
    if ( strcmp( name, "HOST" ) == 0 ) strcpy( host, value ) ;
    else if ( strcmp( name, "Connection" ) == 0 ) strcpy( connection, value ) ;
    else if ( strcmp( name, "Range" ) == 0 ) {
      strcpy( range, name ) ;
      //strcpy( value, value + 6 ) ;  // ���h"bytes="
      sscanf( value, "%*[^=]=%d-%d", &range_start, &range_end ) ; // ���Xstart & end
      //printf( "234: %d-%d\n", range_start, range_end );
    } // else if
    else if ( strcmp( name, "Content-Length" ) == 0 ) {
      bodylen = atoi( value ) ;
    } // else if
    else if ( strcmp( name, "If-Modified-Since" ) == 0 )
      strcpy( If_Modified_Since, value ) ;
    else if ( strcmp( name, "If-Match" ) == 0 ) {
      strcpy( If_Match, value ) ;
    } // else if
    else if ( strcmp( name, "If-None-Match" ) == 0 ) {
      strcpy( If_None_Match, value ) ;
    } // else if
    else if ( strcmp( name, "If-Range" ) == 0 ) {
      strcpy( If_Range, value ) ;
    } // else if
    else if ( strcmp( name, "If-Unmodified-Since" ) == 0 ) {
      strcpy( If_Unmodified_Since, value ) ;
    } // else if
    else if ( strcmp( name, "Expect" ) == 0 ) {
      strcpy( Expect, value ) ;
    } // else if
    else if ( strcmp( buf, "\r\n" ) == 0 ) break ;

    printf( "[request] %s", buf ) ;
  } // for

  printf( "-----------------receiving header end-----------------\n\n" ) ;
  //fp = fopen( filePath, "rb" ) ;

  if ( strcmp( req_path, "/returnURL.html" ) == 0 ) {  // ��������
    statuscode = 301 ;
    printf( "\n301: �����CYCU\n\n" ) ;
    strcat( header, "Location: http://www.cycu.edu.tw/\r\n" ) ;
    strcpy( filePath, "WebTest/301.html" ) ;
    fp = fopen(filePath, "rb" ) ; 
  } // if 
  else if ( strcmp( req_path, "/parameter.html" ) == 0 )
    ;
  else if ( strcmp( req_method, "GET" ) != 0 && strcmp( req_method, "POST" ) != 0 &&
            strcmp( req_method, "HEAD" ) != 0 && strcmp( req_method, "DELETE" ) != 0 &&
            strcmp( req_method, "PUT" ) != 0 ) {
    statuscode = 400 ;
    fp = fopen( "WebTest/400.html", "rb" ) ;
    strcpy( filePath, "WebTest/400.html" ) ;
    printf( "ERROR:400 \n\n" ) ;
  } // else if
  else if ( strcmp( req_ver, "HTTP/1.1" ) != 0 ) {
    statuscode = 505 ;
    //fclose( fp ) ;
    fp = fopen( "WebTest/505.html", "rb" ) ;
    strcpy( filePath, "WebTest/505.html" ) ;
    printf( "ERROR:505 \n\n" ) ;
  } // else if
  else if ( strcmp( req_method, "PUT" ) !=0 && (fp = fopen( filePath, "rb" ) ) == NULL ) { // �}�Ҹ귽�çP�_�s���s�b
    statuscode = 404 ;
    fp = fopen( "WebTest/404.html", "rb" ) ;
    strcpy( filePath, "WebTest/404.html" ) ;
    printf( "ERROR:404 \n\n" ) ;
  } // else if

  //if ( fp == NULL ) fp = fopen( filePath, "rb" ) ; // �}�Ҹ귽


  // *****���o�ɮ׸�T*****
  if ( fp != NULL ) {
  // ���o�ɮפj�p 
    struct stat st;
    stat(filePath, &st);
    filebyte = st.st_size;
    sprintf( filelen,"Content-Length: %ld\r\n", filebyte ) ;
  //strcat( header, filelen ) ; // set Content-Length
  // ���o last modified time
    char temp[100] = "" ;
    struct tm* timeIfo;
    strftime(time, 100, "%a, %d %b %Y %X GMT", localtime( &st.st_mtime ) ) ;
    sprintf( resp_lasttime, "Last-Modified: %s\r\n", time ) ;
    // strcat( header, resp_time ) ;
  } // if


  //printf( "%s", header ) ;

  file_extension = strrchr( filePath, '.' ) ; // ���o���ɦW
  file_name = strrchr( filePath, '/' ) + 1 ;      // ���o�ɦW

  printf( "\n�ɮ׸�T:\n" ) ;
  printf( "�ɮפj�p: %s", filelen ) ;
  printf( "�ɮצW��: %s\n", file_name ) ;
  printf( "�ɮ�����: %s\n", file_extension ) ;
  printf( "�ɮ׳̫�ק���: %s\n", time ) ;

  sprintf( contenttype, "Content-Type: %s\r\n", ContentType( file_extension ) ) ;
  // strcat( header, contenttype ) ; // set Content-Type



  if ( strcmp( req_method, "GET" ) == 0 || strcmp( req_method, "HEAD" ) == 0 ||
       strcmp( req_method, "POST" ) == 0 ) {        // GET, HEAD, POST

    if ( strcmp( req_method, "POST" ) == 0 ) {
      GetBody( &m_socket, req_para, bodylen ) ;  // ���oPOST�᭱��body(�Ѽ�)
      printf( "�Ѽ�: %s\n", req_para ) ;
    } // if

    else if ( statuscode != 404 && strcmp( req_para, "" ) == 0 ) {
      // ********Etag �ͦ�*********
      char nameAndmodified[200] = "" ;   // �O�� file_name + resp_time �r��
      char tag[200] = "" ;           // �O���[�K��r��
      char decode[200] = "" ;
      char temp[200] = "\"" ;
      strcat( nameAndmodified, file_name ) ;
      strcat( nameAndmodified, time ) ;
      b64_encode( nameAndmodified, tag ) ; // base64 �[�K
      strcat( temp, tag ) ;
      temp[strlen(temp)] = '\"' ;
      strcpy( tag, temp ) ;
      sprintf( Etag, "ETag: %s\r\n", tag ) ;
      strcat( header, Etag ) ;
      b64_decode( tag, decode ) ;
      //printf( "decode: %s\n", decode ) ;

      strcat( header, "Accept-Ranges: bytes\r\n" ) ;
      // **********conditional get***********

      //**********If_Modified_Since && If_None_Match**************
      if ( strcmp( If_Modified_Since, "" ) != 0 || strcmp( If_None_Match, "" ) != 0  ) { // If_Modified_Since
        bool if_since = ( strcmp( If_Modified_Since, "" ) == 0 || strcmp( If_Modified_Since, time ) == 0 )  ;
        bool if_Match = ( strcmp( If_None_Match, "" ) == 0 || strcmp( If_None_Match, tag ) == 0 ) ;
        if ( if_since && if_Match ) {
          statuscode = 304 ;
        } // if
        else {
          statuscode = 200 ;
        } // else
      } // else if

      // ***********If-Rang && Range*************
      else if ( ( strcmp( If_Range, "" ) == 0 || strcmp( If_Range, tag ) == 0 )  && strcmp( range, "" ) != 0 ) {
        statuscode = 206 ;
        if ( range_end == 0 )         // case Range: bytes=xxx-
          range_end = filebyte - 1;
        sprintf( resp_range, "Content-Range: bytes %d-%d/%d\r\n", range_start, range_end, filebyte ) ;
        strcat( header, resp_range ) ;
        sprintf( filelen, "Content-Length: %ld\r\n", range_end - range_start + 1 ) ;
        //strcat( header, filelen ) ;
      } // else if

      // **********If-Match*****************
      else if ( strcmp( If_Match, "" ) != 0 && strcmp( If_Match, tag ) != 0 ) {
        statuscode = 412 ;
      } // else if

      //***********If-Unmodified-Since**********
      else if ( strcmp( If_Unmodified_Since, "" ) != 0 && strcmp( If_Unmodified_Since, time ) != 0 ) {
        statuscode = 412 ;
      } // else if 
    } // else if

    else {    // 404
      strcat( header, "Accept-Ranges: bytes\r\n" ) ;
    } // else

    if ( strcmp( req_para, "" ) != 0 ) {  // ���Ѽ�case
      char response[1024] = "" ;
      char key1[100] = "no parameter", key2[100] = "", key3[100] = "" ;
      if ( strcmp( req_para, "par1=&par2=&par3=" ) != 0 )   // ���ѼƦb��
        sscanf( req_para, "%[^&]&%[^&]&%s", key1, key2, key3 ) ;
      printf( "paramater:\n%s\n%s\n%s\n", key1, key2, key3 ) ;
      sprintf( response, "<html><head><title>Parame test</title></head><body> <h1> parameter: <br> <br> %s <br> %s <br> %s </h1> </body></html>", key1, key2, key3 ) ;
      sprintf( filelen, "Content-Length: %d\r\n", strlen( response ) );
      strcat( header, filelen ) ;
      strcat( header, contenttype ) ;
      strcat( header, resp_lasttime ) ;
      SendHeader( &m_socket, statuscode, header ) ;
      send( m_socket, response, strlen( response ), 0 ) ;
    } // if

    else {    // �D�Ѽ�case
      if ( statuscode != 412 && statuscode != 304 ) {
        strcat( header, filelen ) ;
      } // if

      strcat( header, resp_lasttime ) ;
      strcat( header, contenttype ) ;
      SendHeader( &m_socket, statuscode, header ) ;
    
      if ( strcmp( req_method, "HEAD" ) != 0 && statuscode != 304 && statuscode != 412 ){     // head & 304 & 412���^�� body���s����
        if ( range_end == 0 )         // �Y�L�]�wrange_end, �w�]���ɮפj�p
          range_end = filebyte - 1 ;  // �]�w�^���ɮפW��
        fseek( fp, range_start, SEEK_SET ) ;  // �]�w�^���ɮװ_�l��m
        SendContent( fp, &m_socket, range_end - range_start + 1 ) ;
      } // if
    } // else
  } // end if ***** POST, GET, HEAD *********

  else if ( strcmp( req_method, "DELETE" ) == 0 && statuscode == 200 ) {
    if ( fp != NULL ) { // �ˬd�ɮ׬O�_�ϥΤ�
      fclose(fp) ;
      fp = NULL ;
    } // if
    if ( remove(filePath)  == 0 ){ // �R�����\ 
      printf( "delete successs!\n" ) ;
      statuscode = 204 ;
    } // if
    else {                      // �R������
      statuscode = 500 ;
      printf( "delete failed!\n");
    } // else

    SendHeader( &m_socket, statuscode, header ) ;
  } // else if

  else if ( strcmp( req_method, "PUT" ) == 0 ) {
    if ( fp != NULL )
      fclose( fp ) ;
    fp = fopen( filePath, "wb" ) ;
    if ( fp != NULL ) {
      if ( strcmp( Expect, "" ) != 0 ) {
        send( m_socket, "HTTP/1.1 100 Continue\r\n\r\n", strlen( "HTTP/1.1 100 Continue\r\n\r\n" ), 0 ) ;
        printf( "send 100 Continue\n\n" ) ;
      } // if
      printf( "writing..." ) ;
      statuscode = 201 ;
      int datalen = 0 ;
      for ( int i = 1 ; i != 0 && datalen < bodylen && i != SOCKET_ERROR ; datalen = datalen + i ) {
        char buf[BUF_MAX] = "" ; 
        i = recv( m_socket, buf, sizeof(buf), 0 ) ;
        fwrite( buf, sizeof(char), i, fp ) ;
      } // for

      char location[100] = "" ;
      sprintf( location, "Location: http://%s\r\n", filePath ) ;
      strcat( header, location ) ;
      SendHeader( &m_socket, statuscode, header ) ;
    } // if
    else {
      statuscode = 500 ;
      SendHeader( &m_socket, statuscode, header ) ;
    } // else
  } // else if
  else {        // error case
    strcat( header, filelen ) ;
    strcat( header, contenttype ) ;
    SendHeader( &m_socket, statuscode, header ) ;
    if ( fp != NULL )
      SendContent( fp, &m_socket, filebyte ) ;
  } // else

  printf( "---------------- sending end -----------------\n\n\n" ) ;
  if ( fp != NULL )
    fclose( fp ) ;
  closesocket(m_socket);
  delete sockInfo ;
}  // ThreadConnect