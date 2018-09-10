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


function init()
    local header = {
        codec = "json",
        method = "node.register",
        session = "1"
    }
    local payload = cjson.encode({
        name = "s01"
    })
    local headerchunk = encode_header(header)
    local payloadchunk = payload or ""
    local packlist = {}
    local headersize = #headerchunk
    local packsize = headersize + #payloadchunk + 2
    table.insert(packlist, string.pack(">I2", packsize))
    table.insert(packlist, string.pack(">I2", headersize))
    table.insert(packlist, headerchunk)
    table.insert(packlist, payloadchunk)
    req = table.concat(packlist)
end

function request()
    return req
end
