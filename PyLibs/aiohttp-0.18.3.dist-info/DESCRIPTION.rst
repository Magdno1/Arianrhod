http client/server for asyncio
==============================

.. image:: https://raw.github.com/KeepSafe/aiohttp/master/docs/_static/aiohttp-icon-128x128.png
  :height: 64px
  :width: 64px
  :alt: aiohttp logo

.. image:: https://secure.travis-ci.org/KeepSafe/aiohttp.png
  :target:  https://secure.travis-ci.org/KeepSafe/aiohttp
  :align: right

Features
--------

- Supports both client and server side of HTTP protocol.
- Supports both client and server Web-Sockets out-of-the-box.
- Web-server has middlewares and pluggable routing.


Requirements
------------

- Python >= 3.4.1
- chardet https://pypi.python.org/pypi/chardet

Optionally you may install cChardet library:
https://pypi.python.org/pypi/cchardet/1.0.0


License
-------

``aiohttp`` is offered under the Apache 2 license.


Documentation
-------------

http://aiohttp.readthedocs.org/

Source code
------------

The latest developer version is available in a github repository:
https://github.com/KeepSafe/aiohttp

Benchmarks
----------

If you are interested in by efficiency, AsyncIO community maintains a list of benchmarks on the official wiki:
https://github.com/python/asyncio/wiki/Benchmarks

Getting started
---------------

Client
^^^^^^

To retrieve something from the web:

.. code-block:: python

  import aiohttp
  import asyncio

  async def get_body(client, url):
      async with client.get(url) as response:
          return await response.read()

  if __name__ == '__main__':
      loop = asyncio.get_event_loop()
      client = aiohttp.ClientSession(loop=loop)
      raw_html = loop.run_until_complete(get_body(client, 'http://python.org'))
      print(raw_html)
      client.close()


If you want to use timeouts for aiohttp client please use standard
asyncio approach:

.. code-block:: python

   yield from asyncio.wait_for(client.get(url), 10)


Server
^^^^^^

This is simple usage example:

.. code-block:: python

    import asyncio
    from aiohttp import web

    async def handle(request):
        name = request.match_info.get('name', "Anonymous")
        text = "Hello, " + name
        return web.Response(body=text.encode('utf-8'))

    async def wshandler(request):
        ws = web.WebSocketResponse()
        ws.start(request)

        while True:
            msg = await ws.receive()

            if msg.tp == web.MsgType.text:
                ws.send_str("Hello, {}".format(msg.data))
            elif msg.tp == web.MsgType.binary:
                ws.send_bytes(msg.data)
            elif msg.tp == web.MsgType.close:
                break

        return ws


    async def init(loop):
        app = web.Application(loop=loop)
        app.router.add_route('GET', '/echo', wshandler)
        app.router.add_route('GET', '/{name}', handle)

        srv = await loop.create_server(app.make_handler(),
                                            '127.0.0.1', 8080)
        print("Server started at http://127.0.0.1:8080")
        return srv

    loop = asyncio.get_event_loop()
    loop.run_until_complete(init(loop))
    loop.run_forever()


Note: examples are written for Python 3.5+ and utilize PEP-492 aka
async/await.  If you are using Python 3.4 please replace `await` with
`yield from` and `async def` with `@coroutine` e.g.::

    async def coro(...):
        ret = await f()

shoud be replaced by::

    @asyncio.coroutine
    def coro(...):
        ret = yield from f()

CHANGES
=======

0.18.3 (25-10-2015)
-------------------

- Fix formatting for _RequestContextManager helper #590 

0.18.2 (22-10-2015)
-------------------

- Fix regression for OpenSSL < 1.0.0 #583

0.18.1 (20-10-2015)
-------------------

- Relax rule for router names: they may contain dots and columns
  starting from now

0.18.0 (19-10-2015)
-------------------

- Use errors.HttpProcessingError.message as HTTP error reason and
  message #459

- Optimize cythonized multidict a bit

- Change repr's of multidicts and multidict views

- default headers in ClientSession are now case-insensitive

- Make '=' char and 'wss://' schema safe in urls #477

- `ClientResponse.close()` forces connection closing by default from now #479
  N.B. Backward incompatible change: was `.close(force=False)
  Using `force` parameter for the method is deprecated: use `.release()` instead.

- Properly requote URL's path #480

- add `skip_auto_headers` parameter for client API #486

- Properly parse URL path in aiohttp.web.Request #489

- Raise RuntimeError when chunked enabled and HTTP is 1.0 #488

- Fix a bug with processing io.BytesIO as data parameter for client API #500

- Skip auto-generation of Content-Type header #507

- Use sendfile facility for static file handling #503

- Default `response_factory` in `app.router.add_static` now is
  `StreamResponse`, not `None`. The functionality is not changed if
  default is not specified.

- Drop `ClientResponse.message` attribute, it was always implementation detail.

- Streams are optimized for speed and mostly memory in case of a big
  HTTP message sizes #496

- Fix a bug for server-side cookies for dropping cookie and setting it
  again without Max-Age parameter.

- Don't trim redirect URL in client API #499

- Extend precision of access log "D" to milliseconds #527

- Deprecate `StreamResponse.start()` method in favor of
  `StreamResponse.prepare()` coroutine #525

  `.start()` is still supported but responses begun with `.start()`
  doesn't call signal for response preparing to be sent. 

- Add `StreamReader.__repr__`

- Drop Python 3.3 support, from now minimal required version is Python
  3.4.1 #541

- Add `async with` support for `ClientSession.request()` and family #536

- Ignore message body on 204 and 304 responses #505

- `TCPConnector` processed both IPv4 and IPv6 by default #559

- Add `.routes()` view for urldispatcher #519

- Route name should be a valid identifier name from now #567

- Implement server signals #562

- Drop an year-old deprecated *files* parameter from client API.

- Added `async for` support for aiohttp stream #542

