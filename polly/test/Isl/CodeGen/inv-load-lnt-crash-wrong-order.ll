; RUN: opt %loadPolly -polly-codegen -S < %s | FileCheck %s
;
; This crashed our codegen at some point, verify it runs through
;
; CHECK: polly.start
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

%struct.EqState.41.74.107.338.503.866.932.965.998.1064.2052.2151.2184.2606 = type { %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605* }
%struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588 = type { i32, %struct.Production.29.62.95.326.491.854.920.953.986.1052.2040.2139.2172.2584*, i32, i32, i32, i32, %struct.anon.0.30.63.96.327.492.855.921.954.987.1053.2041.2140.2173.2585, %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*, %struct.Code.31.64.97.328.493.856.922.955.988.1054.2042.2141.2174.2586, %struct.Code.31.64.97.328.493.856.922.955.988.1054.2042.2141.2174.2586, %struct.anon.1.32.65.98.329.494.857.923.956.989.1055.2043.2142.2175.2587, i32, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588* }
%struct.Production.29.62.95.326.491.854.920.953.986.1052.2040.2139.2172.2584 = type { i8*, i32, %struct.anon.9.42.75.306.471.834.900.933.966.1032.2020.2119.2152.2581, i32, i8, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*, [8 x %struct.Production.29.62.95.326.491.854.920.953.986.1052.2040.2139.2172.2584*], [8 x %struct.Declaration.13.46.79.310.475.838.904.937.970.1036.2024.2123.2156.2582*], %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*, %struct.Term.18.51.84.315.480.843.909.942.975.1041.2029.2128.2161.2583*, %struct.Production.29.62.95.326.491.854.920.953.986.1052.2040.2139.2172.2584* }
%struct.anon.9.42.75.306.471.834.900.933.966.1032.2020.2119.2152.2581 = type { i32, i32, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588**, [3 x %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*] }
%struct.Declaration.13.46.79.310.475.838.904.937.970.1036.2024.2123.2156.2582 = type { %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*, i32, i32 }
%struct.Term.18.51.84.315.480.843.909.942.975.1041.2029.2128.2161.2583 = type { i32, i32, i32, i32, i32, i8*, i32, i8, %struct.Production.29.62.95.326.491.854.920.953.986.1052.2040.2139.2172.2584* }
%struct.anon.0.30.63.96.327.492.855.921.954.987.1053.2041.2140.2173.2585 = type { i32, i32, %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591**, [3 x %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*] }
%struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591 = type { i32, i32, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*, %union.anon.11.44.77.308.473.836.902.935.968.1034.2022.2121.2154.2590 }
%union.anon.11.44.77.308.473.836.902.935.968.1034.2022.2121.2154.2590 = type { %struct.Unresolved.10.43.76.307.472.835.901.934.967.1033.2021.2120.2153.2589 }
%struct.Unresolved.10.43.76.307.472.835.901.934.967.1033.2021.2120.2153.2589 = type { i8*, i32 }
%struct.Code.31.64.97.328.493.856.922.955.988.1054.2042.2141.2174.2586 = type { i8*, i32 }
%struct.anon.1.32.65.98.329.494.857.923.956.989.1055.2043.2142.2175.2587 = type { i32, i32, %struct.Code.31.64.97.328.493.856.922.955.988.1054.2042.2141.2174.2586**, [3 x %struct.Code.31.64.97.328.493.856.922.955.988.1054.2042.2141.2174.2586*] }
%struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605 = type { i32, i64, %struct.anon.2.14.47.80.311.476.839.905.938.971.1037.2025.2124.2157.2592, %struct.anon.3.15.48.81.312.477.840.906.939.972.1038.2026.2125.2158.2593, %struct.VecGoto.17.50.83.314.479.842.908.941.974.1040.2028.2127.2160.2595, %struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597, %struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597, %struct.VecHint.22.55.88.319.484.847.913.946.979.1045.2033.2132.2165.2599, %struct.VecHint.22.55.88.319.484.847.913.946.979.1045.2033.2132.2165.2599, %struct.Scanner.27.60.93.324.489.852.918.951.984.1050.2038.2137.2170.2604, i8, i8*, i32, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588* }
%struct.anon.2.14.47.80.311.476.839.905.938.971.1037.2025.2124.2157.2592 = type { i32, i32, %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591**, [3 x %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*] }
%struct.anon.3.15.48.81.312.477.840.906.939.972.1038.2026.2125.2158.2593 = type { i32, i32, %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591**, [3 x %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*] }
%struct.VecGoto.17.50.83.314.479.842.908.941.974.1040.2028.2127.2160.2595 = type { i32, i32, %struct.Goto.16.49.82.313.478.841.907.940.973.1039.2027.2126.2159.2594**, [3 x %struct.Goto.16.49.82.313.478.841.907.940.973.1039.2027.2126.2159.2594*] }
%struct.Goto.16.49.82.313.478.841.907.940.973.1039.2027.2126.2159.2594 = type { %struct.Elem.12.45.78.309.474.837.903.936.969.1035.2023.2122.2155.2591*, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605* }
%struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597 = type { i32, i32, %struct.Action.19.52.85.316.481.844.910.943.976.1042.2030.2129.2162.2596**, [3 x %struct.Action.19.52.85.316.481.844.910.943.976.1042.2030.2129.2162.2596*] }
%struct.Action.19.52.85.316.481.844.910.943.976.1042.2030.2129.2162.2596 = type { i32, %struct.Term.18.51.84.315.480.843.909.942.975.1041.2029.2128.2161.2583*, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, i32, i8* }
%struct.VecHint.22.55.88.319.484.847.913.946.979.1045.2033.2132.2165.2599 = type { i32, i32, %struct.Hint.21.54.87.318.483.846.912.945.978.1044.2032.2131.2164.2598**, [3 x %struct.Hint.21.54.87.318.483.846.912.945.978.1044.2032.2131.2164.2598*] }
%struct.Hint.21.54.87.318.483.846.912.945.978.1044.2032.2131.2164.2598 = type { i32, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588* }
%struct.Scanner.27.60.93.324.489.852.918.951.984.1050.2038.2137.2170.2604 = type { %struct.VecScanState.25.58.91.322.487.850.916.949.982.1048.2036.2135.2168.2602, %struct.VecScanStateTransition.26.59.92.323.488.851.917.950.983.1049.2037.2136.2169.2603 }
%struct.VecScanState.25.58.91.322.487.850.916.949.982.1048.2036.2135.2168.2602 = type { i32, i32, %struct.ScanState.24.57.90.321.486.849.915.948.981.1047.2035.2134.2167.2601**, [3 x %struct.ScanState.24.57.90.321.486.849.915.948.981.1047.2035.2134.2167.2601*] }
%struct.ScanState.24.57.90.321.486.849.915.948.981.1047.2035.2134.2167.2601 = type { i32, [256 x %struct.ScanState.24.57.90.321.486.849.915.948.981.1047.2035.2134.2167.2601*], %struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597, %struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597, [256 x %struct.ScanStateTransition.23.56.89.320.485.848.914.947.980.1046.2034.2133.2166.2600*] }
%struct.ScanStateTransition.23.56.89.320.485.848.914.947.980.1046.2034.2133.2166.2600 = type { i32, %struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597, %struct.VecAction.20.53.86.317.482.845.911.944.977.1043.2031.2130.2163.2597 }
%struct.VecScanStateTransition.26.59.92.323.488.851.917.950.983.1049.2037.2136.2169.2603 = type { i32, i32, %struct.ScanStateTransition.23.56.89.320.485.848.914.947.980.1046.2034.2133.2166.2600**, [3 x %struct.ScanStateTransition.23.56.89.320.485.848.914.947.980.1046.2034.2133.2166.2600*] }

; Function Attrs: nounwind
declare noalias i8* @malloc() #0

; Function Attrs: nounwind uwtable
define void @build_eq() #1 {
entry:
  %call = tail call noalias i8* @malloc() #2
  %0 = bitcast i8* %call to %struct.EqState.41.74.107.338.503.866.932.965.998.1064.2052.2151.2184.2606*
  br label %for.cond.preheader

for.cond.preheader:                               ; preds = %for.cond.preheader, %entry
  br i1 undef, label %for.cond.316.preheader, label %for.cond.preheader

for.cond.316.preheader:                           ; preds = %for.cond.preheader
  br i1 undef, label %for.cond.400.preheader, label %for.body.321

for.cond.400.preheader:                           ; preds = %for.inc.397, %for.cond.316.preheader
  br i1 undef, label %for.end.423, label %for.body.405

for.body.321:                                     ; preds = %for.inc.397, %for.cond.316.preheader
  %eq329 = getelementptr inbounds %struct.EqState.41.74.107.338.503.866.932.965.998.1064.2052.2151.2184.2606, %struct.EqState.41.74.107.338.503.866.932.965.998.1064.2052.2151.2184.2606* %0, i64 0, i32 0
  br i1 undef, label %for.inc.397, label %land.lhs.true.331

land.lhs.true.331:                                ; preds = %for.body.321
  br i1 undef, label %for.inc.397, label %if.then.334

if.then.334:                                      ; preds = %land.lhs.true.331
  %1 = load %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605** %eq329, align 8
  %2 = load %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588*, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588** undef, align 8
  br i1 undef, label %for.inc.397, label %land.lhs.true.369

land.lhs.true.369:                                ; preds = %if.then.334
  %n380 = getelementptr inbounds %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588, %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588* %2, i64 0, i32 6, i32 0
  %3 = load i32, i32* %n380, align 8
  %cmp381 = icmp eq i32 %3, 2
  br i1 %cmp381, label %if.then.383, label %for.inc.397

if.then.383:                                      ; preds = %land.lhs.true.369
  %reduces_to385 = getelementptr inbounds %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605* %1, i64 0, i32 14
  store %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605* undef, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605** %reduces_to385, align 8
  %diff_rule386 = getelementptr inbounds %struct.EqState.41.74.107.338.503.866.932.965.998.1064.2052.2151.2184.2606, %struct.EqState.41.74.107.338.503.866.932.965.998.1064.2052.2151.2184.2606* %0, i64 0, i32 1
  %4 = bitcast %struct.Rule.33.66.99.330.495.858.924.957.990.1056.2044.2143.2176.2588** %diff_rule386 to i64*
  %5 = load i64, i64* %4, align 8
  %6 = load %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605*, %struct.State.28.61.94.325.490.853.919.952.985.1051.2039.2138.2171.2605** %eq329, align 8
  br label %for.inc.397

for.inc.397:                                      ; preds = %if.then.383, %land.lhs.true.369, %if.then.334, %land.lhs.true.331, %for.body.321
  br i1 undef, label %for.body.321, label %for.cond.400.preheader

for.body.405:                                     ; preds = %for.cond.400.preheader
  unreachable

for.end.423:                                      ; preds = %for.cond.400.preheader
  ret void
}
