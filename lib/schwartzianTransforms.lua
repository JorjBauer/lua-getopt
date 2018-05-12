function table.dump(o,depth)
   if depth==nil then depth=2 end
   if type(o) == "table" then
      local s = "{\n"
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = ""..k.."" end
	 for i=0,depth do
	    s = s .. ' '
	 end
         s = s .. '[' ..k..'] = ' .. table.dump(v,depth+2) .. ",\n"
      end
      for i=0,depth-2 do
	 s = s .. ' '
      end
      return s..'}'
   elseif type(o) == nil then
      return "<nil>"
   else
      return '"' .. tostring(o):gsub('"', '\"') .. '"' -- lame attempt at proper quoting/escaping
   end
end

local _origSort = table.sort

function table:sort(f)
   _origSort(self,f)
   return self
end

function table:keys()
   local keys = {}
   for key in pairs(self) do
      table.insert(keys, key)
   end
   return keys
end

function table:values()
   local values = {}
   for _,val in pairs(self) do
      table.insert(values, val)
   end
   return values
end

-- take in a list; call the function for each element; aggregate
-- results and return
function table:map(f)
   local r = {}
   for _,val in pairs(self) do
      f(self, val, r)
   end
   return r
end
