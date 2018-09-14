-- socat -v tcp-l:8181,fork exec:"/bin/cat"
-- ./wrk -c1 -t1 -d1s -s scripts/srpc.lua srpc://127.1:8181

function init()
    req = wrk.format_srpc("node.register", {name = "s01"})
    -- print("#req = ", #req)
end

function request()
    return req
end

function response(headers, body)
    -- local h = wrk.decode_header(headers)
    -- local ok, tb = wrk.decode_body(body)
    -- print(ok, h.method, h.session, tb and tb.name)
    -- print(ok, h.code)
end
