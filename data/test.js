

function newTestObj() {
  var result = {};
  result.x = 10;
  return result;
}

obj = newTestObj();
obj.x = obj.x * obj.x;
print(obj.x);

// return result
obj.x;