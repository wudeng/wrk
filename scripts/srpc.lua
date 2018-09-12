function init()
    req = wrk.format_srpc("node.register", {name = "s01"})
end

function request()
    return req
end
