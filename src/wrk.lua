local cjson = require "cjson"
local ktt = {} -- key type table
ktt[1] = "codec";        ktt["codec"]           = 1
ktt[2] = "method";       ktt["method"]          = 2
ktt[3] = "session";      ktt["session"]         = 3
ktt[4] = "code";         ktt["code"]            = 4
ktt[5] = "error";        ktt["error"]           = 5
ktt[6] = "timestamp";    ktt["timestamp"]       = 6
ktt[7] = "trace";        ktt["trace"]           = 7
ktt[8] = "proxy";        ktt["proxy"]           = 8

local vtt = {} -- value type table
vtt[1] = "json";         vtt["json"]            = 1
vtt[2] = "sproto";       vtt["sproto"]          = 2
vtt[3] = "protobuf";     vtt["protobuf"]        = 3
vtt[4] = "raw";          vtt["raw"]             = 4
vtt[5] = "0";            vtt["0"]               = 5

local function encode_key(k)
    k = tostring(k)
    local kt = ktt[k]
    if kt then
        return string.pack("B", 0x80 | kt)
    else
        local lenk = #k
        assert(lenk < 0x7f, "key length larger than 127: ".. k)
        return string.pack(">s1", k)
    end
end

local function encode_value(v)
    v = tostring(v)
    local vt = vtt[v]
    if vt then
        return string.pack("B", 0x80 | vt)
    else
        local lenv = #v
        if lenv < 0x40 then
            return string.pack(">s1", v)
        elseif lenv < 0x4000 then
            return string.pack(string.format(">BBc%s", lenv), 0x40 | (lenv >> 8), lenv & 0xff, v)
        else
            error(string.format("Not supported string length, %s", v))
        end
    end
end

local function encode_header(header)
    local packlist = {}
    for k, v in pairs(header) do
        table.insert(packlist, encode_key(k))
        table.insert(packlist, encode_value(v))
    end
    return table.concat(packlist)
end


local wrk = {
   scheme  = "http",
   host    = "localhost",
   port    = nil,
   method  = "GET",
   path    = "/",
   headers = {},
   body    = nil,
   thread  = nil,
}

function wrk.resolve(host, service)
   local addrs = wrk.lookup(host, service)
   for i = #addrs, 1, -1 do
      if not wrk.connect(addrs[i]) then
         table.remove(addrs, i)
      end
   end
   wrk.addrs = addrs
end

function wrk.setup(thread)
   thread.addr = wrk.addrs[1]
   if type(setup) == "function" then
      setup(thread)
   end
end

function wrk.init(args)
   if not wrk.headers["Host"] then
      local host = wrk.host
      local port = wrk.port

      host = host:find(":") and ("[" .. host .. "]")  or host
      host = port           and (host .. ":" .. port) or host

      wrk.headers["Host"] = host
   end

   if type(init) == "function" then
      init(args)
   end

   local req = wrk.format()
   wrk.request = function()
      return req
   end
end

-- 构造用于发送的http请求
function wrk.format(method, path, headers, body)
   local method  = method  or wrk.method
   local path    = path    or wrk.path
   local headers = headers or wrk.headers
   local body    = body    or wrk.body
   local s       = {}

   if not headers["Host"] then
      headers["Host"] = wrk.headers["Host"]
   end

   headers["Content-Length"] = body and string.len(body)

   s[1] = string.format("%s %s HTTP/1.1", method, path)
   for name, value in pairs(headers) do
      s[#s+1] = string.format("%s: %s", name, value)
   end

   s[#s+1] = ""
   s[#s+1] = body or ""

   return table.concat(s, "\r\n")
end

function wrk.format_srpc(method, data)
    local header = {
        codec = "json",
        method = method,
        session = "1"
    }
    local payload = cjson.encode(data)
    local headerchunk = encode_header(header)
    local payloadchunk = payload or ""
    local packlist = {}
    local headersize = #headerchunk
    local packsize = headersize + #payloadchunk + 2
    table.insert(packlist, string.pack(">I2", packsize))
    table.insert(packlist, string.pack(">I2", headersize))
    table.insert(packlist, headerchunk)
    table.insert(packlist, payloadchunk)
    return table.concat(packlist)
end

return wrk
