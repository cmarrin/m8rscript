//
// Timing test
//

var a = [ ];
var n = 4000;
a.length = n;

print("\n\nTiming test: " + n + " squared iterations\n");

var startTime = Date.now();

for (var i = 0; i < n; ++i) {
    for (var j = 0; j < n; ++j) {
        var f = 3;
        a[j] = j * (j + 1) / 2;
    }
}

var t = Date.now() - startTime;
print("Run time: " + (t) + "ms\n\n");
