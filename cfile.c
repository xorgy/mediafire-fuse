#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <curl/curl.h>

#include "cfile.h"
#include "strings.h"

struct _cfile_s
{
    CURL        *curl_handle;
    char        *url;

    int         mode;
    uint32_t    opts;

    char        *get_args;
    char        *post_args;

    size_t      (*curl_rx_io)   (char *,size_t,size_t,void *);
    size_t      (*curl_tx_io)   (char *,size_t,size_t,void *);

    void        (*user_io)      (cfile_t *);
    void        *userptr;

    void        (*progress)     (cfile_t *);

    char        *rx_buffer;
    ssize_t     rx_buffer_sz;

    char        *tx_buffer;
    ssize_t     tx_buffer_sz;

    double      rx_length;
    double      rx_count;
    double      tx_length;
    double      tx_count;
};

/*
    according to libcurl doc the payload of these two callbacks (data)
    is size * nmemb which is logistically similar to calloc().
*/

/*
static size_t
curl_send_callback(char *data,size_t size,size_t nmemb,void *user_ptr);
*/

static size_t
_cfile_rx_callback(char *data,size_t size,size_t nmemb,void *user_ptr);

static size_t
_cfile_tx_callback(char *data,size_t size,size_t nmemb,void *user_ptr);

static int
_cfile_progress_callback(void *user_ptr,double dltotal,double dlnow,
    double ultotal,double ulnow);


cfile_t*
cfile_create(void)
{
    cfile_t     *cfile;
    CURL        *curl_handle;

    curl_handle = curl_easy_init();

    if(curl_handle == NULL) return NULL;

    cfile = (cfile_t*)calloc(1,sizeof(cfile_t));

    cfile->curl_handle = curl_handle;

    cfile->curl_tx_io = &_cfile_tx_callback;
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_READFUNCTION,cfile->curl_tx_io);
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_READDATA,(void*)cfile);

    cfile->curl_rx_io = &_cfile_rx_callback;
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_WRITEFUNCTION,cfile->curl_rx_io);
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_WRITEDATA,(void*)cfile);

    // enable progress reporting
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_NOPROGRESS,0);
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_PROGRESSFUNCTION,_cfile_progress_callback);
    curl_easy_setopt(cfile->curl_handle,
        CURLOPT_PROGRESSDATA,(void*)cfile);

    return cfile;
}

int
cfile_exec(cfile_t *cfile)
{
    char    *url = NULL;
    size_t  len;
    int     retval;

    if(cfile == NULL) return -1;
    if(cfile->curl_handle == NULL) return -1;

    // use an appended URL if GET arguments are supplied
    if(cfile->get_args != NULL)
    {
        len = strlen(cfile->get_args) + strlen(cfile->url) + 1;

        url = (char*)calloc(1,len);

        strcpy(url,cfile->url);
        strcat(url,cfile->get_args);

        curl_easy_setopt(cfile->curl_handle,CURLOPT_URL,url);
    }

    // has the user supplied POST arguments?
    if(cfile->post_args != NULL)
    {
        curl_easy_setopt(cfile->curl_handle,CURLOPT_POSTFIELDS,
            cfile->post_args);
    }

    retval = curl_easy_perform(cfile->curl_handle);

    // restore the URL to the cURL handle if it was modified
    if(url != NULL)
    {
        curl_easy_setopt(cfile->curl_handle,CURLOPT_URL,cfile->url);
        free(url);

        url = NULL;
    }

    /*
        if the user specified CFILE_MODE_TEXT, ensure that the buffer is
        NULL terminated.
    */
    if(cfile->mode == CFILE_MODE_TEXT)
    {
        if(cfile->rx_buffer[cfile->rx_buffer_sz - 1] != '\0')
        {
            cfile->rx_buffer = (char*)realloc(cfile->rx_buffer,
                cfile->rx_buffer_sz + 1);

            cfile->rx_buffer_sz++;

            cfile->rx_buffer[cfile->rx_buffer_sz - 1] = '\0';
        }
    }

    return retval;
}

void
cfile_destroy(cfile_t *cfile)
{
    if(cfile == NULL) return;

    if(cfile->url != NULL) free(cfile->url);
    if(cfile->get_args != NULL) free(cfile->get_args);
    if(cfile->post_args != NULL) free(cfile->post_args);
    if(cfile->rx_buffer != NULL) free(cfile->rx_buffer);
    if(cfile->tx_buffer != NULL) free(cfile->tx_buffer);

    if(cfile->curl_handle != NULL)
    {
        curl_easy_cleanup(cfile->curl_handle);
    }

    free(cfile);
}

void
cfile_set_mode(cfile_t *cfile,int mode)
{
    if(cfile == NULL) return;
    if(mode <= 0) return;

    /*
        CFILE_MODE_TEXT ensure that the final buffer contents is NULL
        terminated.

        CFILE_MODE_BINARY does not modify the contents of the final
        buffer at all.
    */

    cfile->mode = mode;

    return;
}

void
cfile_set_defaults(cfile_t *cfile)
{
    if(cfile == NULL) return;

    cfile->mode = CFILE_MODE_TEXT;

    // if the protocol supports following redirects, do so
    cfile_set_opts(cfile,CFILE_OPT_FOLLOW_REDIRECTS);

    // enable SSL engine
    cfile_set_opts(cfile,CFILE_OPT_ENABLE_SSL);

    return;
}

void
cfile_set_url(cfile_t *cfile,const char *url)
{
    if(cfile == NULL) return;
    if(url == NULL) return;

    // make sure curl handle was successfully instantiated
    if(cfile->curl_handle == NULL) return;

    // reject the request if a source already exists
    if(cfile->url != NULL) return;

    // escape the url
    // cfile->url = curl_easy_escape(cfile->url,url,0);

    cfile->url = strdup(url);

    curl_easy_setopt(cfile->curl_handle,CURLOPT_URL,cfile->url);

    return;
}

int
cfile_get_url(cfile_t *cfile,char *buf,size_t buf_sz)
{
    if(cfile == NULL) return -1;
    if(cfile->url == NULL) return -1;
    if(buf == NULL) return -1;
    if(buf_sz <=1 ) return -1;

    strncpy(buf,cfile->url,buf_sz-1);

    return 0;
}

void
cfile_set_args(cfile_t *cfile,int type,const char *args)
{
    int len;

    if(cfile == NULL) return;
    if(args == NULL) return;
    if(cfile->curl_handle == NULL) return;

    len = strlen(args);
    if(len == 0) return;

    if(type == CFILE_ARGS_POST)
    {
        cfile->post_args = (char*)realloc(cfile->post_args,len + 1);
        strcpy(cfile->post_args,args);
        return;
    }

    if(type == CFILE_ARGS_GET)
    {
        cfile->get_args = (char*)realloc(cfile->get_args,len + 1);
        strcpy(cfile->get_args,args);
        return;
    }

    // todo:  handle custome headers

    return;
}

void
cfile_add_args(cfile_t *cfile,int type,const char *args)
{
    char    *new_args;

    if(cfile == NULL) return;

    if(type == CFILE_ARGS_POST)
    {
        if(cfile->post_args == NULL)
        {
            cfile_set_args(cfile,CFILE_ARGS_POST,args);
            return;
        }

        new_args = strdup_printf("%s%s",cfile->post_args,(char*)args);
        free(cfile->post_args);
        cfile->post_args = new_args;

        return;
    }

    if(type == CFILE_ARGS_GET)
    {
        if(cfile->get_args == NULL)
        {
            cfile_set_args(cfile,CFILE_ARGS_GET,args);
            return;
        }

        new_args = strdup_printf("%s%s",cfile->get_args,(char*)args);
        free(cfile->get_args);
        cfile->get_args = new_args; 

        return;
    }

    return;
}

int
cfile_set_opts(cfile_t *cfile,uint32_t opts)
{
    if(cfile == NULL) return -1;
    if(cfile->curl_handle == NULL) return -1;

    cfile->opts = opts;

    if(cfile->opts & CFILE_OPT_FOLLOW_REDIRECTS)
    {
        curl_easy_setopt(cfile->curl_handle,CURLOPT_FOLLOWLOCATION,1);
    }
    else
    {
        curl_easy_setopt(cfile->curl_handle,CURLOPT_FOLLOWLOCATION,0);
    }

    if(cfile->opts & CFILE_OPT_ENABLE_SSL)
    {
        curl_easy_setopt(cfile->curl_handle,CURLOPT_SSLENGINE,NULL);
        curl_easy_setopt(cfile->curl_handle,CURLOPT_SSLENGINE_DEFAULT,1L);

        // libcurl will use built-in CA certificate if none specified

        //curl_easy_setopt(cfile->curl_handle,CURLOPT_SSLCERTTYPE,"PEM");
        //curl_easy_setopt(cfile->curl_handle,CURLOPT_CAINFO,"cacert.pem");
    }

    if(cfile->opts & CFILE_OPT_ENABLE_SSL_LAX)
    {
        curl_easy_setopt(cfile->curl_handle,CURLOPT_SSL_VERIFYPEER,0);
    }

    return 0;
}

void
cfile_set_io_func(cfile_t *cfile,CFileFunc user_io)
{
    if(cfile == NULL) return;

    cfile->user_io = user_io;

    return;
}

void
cfile_set_progress_func(cfile_t *cfile,CFileFunc progress)
{
    if(cfile == NULL) return;

    cfile->progress = progress;

    return;
}

const char*
cfile_get_rx_buffer(cfile_t *cfile)
{
    if(cfile == NULL) return NULL;
    if(cfile->curl_handle == NULL) return NULL;

    return cfile->rx_buffer;
}

size_t
cfile_get_rx_buffer_size(cfile_t *cfile)
{
    if(cfile == NULL) return 0;

    return cfile->rx_buffer_sz;
}

const char*
cfile_get_tx_buffer(cfile_t *cfile)
{
    if(cfile == NULL) return NULL;
    if(cfile->curl_handle == NULL) return NULL;

    return cfile->tx_buffer;
}

size_t
cfile_get_tx_buffer_size(cfile_t *cfile)
{
    if(cfile == NULL) return 0;

    return cfile->tx_buffer_sz;
}

void
cfile_reset_rx_buffer(cfile_t *cfile)
{
    if(cfile == NULL) return;

    if(cfile->rx_buffer != NULL)
    {
        free(cfile->rx_buffer);
        cfile->rx_buffer = NULL;
    }

    cfile->rx_buffer_sz = 0;

    return;
}

void
cfile_reset_tx_buffer(cfile_t *cfile)
{
    if(cfile == NULL) return;

    if(cfile->tx_buffer != NULL)
    {
        free(cfile->tx_buffer);
        cfile->tx_buffer = NULL;
    }

    cfile->tx_buffer_sz = 0;

    return;
}

double
cfile_get_rx_count(cfile_t *cfile)
{
    if(cfile == NULL) return 0;
    if(cfile->curl_handle == NULL) return 0;

    return cfile->rx_count;
}

double
cfile_get_rx_length(cfile_t *cfile)
{
    if(cfile == NULL) return 0;
    if(cfile->curl_handle == NULL) return 0;

    return cfile->rx_length;
}


double
cfile_get_tx_count(cfile_t *cfile)
{
    if(cfile == NULL) return 0;
    if(cfile->curl_handle == NULL) return 0;

    return cfile->tx_count;
}

double
cfile_get_tx_length(cfile_t *cfile)
{
    if(cfile == NULL) return 0;
    if(cfile->curl_handle == NULL) return 0;

    return cfile->tx_length;
}

void
cfile_set_userptr(cfile_t *cfile,void *anything)
{
    if(cfile == NULL) return;

    cfile->userptr = anything;

    return;
}

void*
cfile_get_userptr(cfile_t *cfile)
{
    if(cfile == NULL) return NULL;

    return cfile->userptr;
}

static size_t
_cfile_tx_callback(char *data,size_t size,size_t nmemb,void *user_ptr)
{
    cfile_t     *cfile;
    size_t      data_sz = 0;
    char        *buffer_tail;

/*
    Your function must return the actual number of bytes that you stored
    in that memory area. Returning 0 will signal end-of-file to the
    library and cause it to stop the current transfer.
*/
    if(user_ptr == NULL) return 0;

    cfile = (cfile_t*)user_ptr;
    data_sz = size * nmemb;

    if(data_sz > 0)
    {
        cfile->tx_buffer = (char*)realloc(cfile->tx_buffer,
            cfile->tx_buffer_sz);

        buffer_tail = cfile->tx_buffer + cfile->tx_buffer_sz;

        memcpy(buffer_tail,data,data_sz);

        cfile->tx_buffer_sz += data_sz;
    }

    if(cfile->user_io != NULL)
    {
        cfile->user_io(cfile);
    }

    if(cfile->progress != NULL)
    {
        cfile->progress(cfile);
    }

    return data_sz;
}

static size_t
_cfile_rx_callback(char *data,size_t size,size_t nmemb,void *user_ptr)
{
    cfile_t     *cfile;
    size_t      data_sz = 0;
    char        *buffer_tail = NULL;

/*
    Your function must return the actual number of bytes that you stored
    in that memory area. Returning 0 will signal end-of-file to the
    library and cause it to stop the current transfer.
*/
    if(user_ptr == NULL) return 0;

    cfile = (cfile_t*)user_ptr;
    data_sz = size * nmemb;

    if(data_sz > 0)
    {
        cfile->rx_buffer = (char*)realloc(cfile->rx_buffer,
            cfile->rx_buffer_sz + data_sz);

        buffer_tail = cfile->rx_buffer + cfile->rx_buffer_sz;

        memcpy(buffer_tail,data,data_sz);

        cfile->rx_buffer_sz += data_sz;
    }

    if(cfile->user_io != NULL)
    {
        cfile->user_io(cfile);
    }

    if(cfile->progress != NULL)
    {
        cfile->progress(cfile);
    }

    return data_sz;
}



static int
_cfile_progress_callback(void *user_ptr,double dltotal,double dlnow,
    double ultotal,double ulnow)
{
    cfile_t     *cfile;

    if(user_ptr == NULL) return 0;

    cfile = (cfile_t*)user_ptr;

    cfile->tx_length = ultotal;
    cfile->tx_count = ulnow;

    cfile->rx_length = dltotal;
    cfile->rx_count = dlnow;

    return 0;
}

