# wrk for srpc

wrk是原本基于LuaJIT的，但是由于srpc的编解码用到了大量Lua5.3的特性，所以将LuaJIT替换成了Lua5.3。

## srpc

目前只支持消息体基于json编码。

## 编译

依赖lua-cjson库。

```
git submodule init
git submodule update
make
```

## 请求

请求需要通过lua脚本来构造。指定method以及参数。

```lua
function init()
    req = wrk.format("node.register", {name = "s01"})
end

function request()
    return req
end
```

init只在初始化的时候执行一次，而request每次请求都会执行。如果是静态的请求，可以通过这种方式来提高性能。

其实这种情况是静态请求，但是wrk仍然会把它当成动态请求来处理。系统目前是通过检查是否存在request函数来判断是否静态请求，可以通过其他地方来手动指定，以提高静态请求的性能。

## 回应

通过wrk模块提供了一些解码的函数。

* decode_header(headers)
* decode_body(body)

```lua
function response(headers, body)
    local h = wrk.decode_header(headers)
    local ok, tb = wrk.decode_body(body)
    -- print(ok, h.method, h.session, tb and tb.name)
    print(ok, h.code)
end
```

这个函数可以用来验证正确性。每个回应都会调用的，所以会有一定的性能损失。压测服务器的时候可以去掉。

## 测试

因为srpc的消息格式并不区分请求和回应，所以简单搭一个echo服务器就可以跑起来。

启动echo服务器：

```bash
socat -v tcp-l:8181,fork exec:"/bin/cat"
```

启动压测：

```bash
./wrk -c1 -t1 -d1s -s scripts/srpc.lua srpc://127.1:8181
```
