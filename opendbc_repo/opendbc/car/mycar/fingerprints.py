# ruff: noqa: E501
from opendbc.car.structs import CarParams
from opendbc.car.mycar.values import CAR
Ecu = CarParams.Ecu
#车辆指纹
# Todo: could fingerprints for song dmi be able to combine?
FINGERPRINTS = {
   CAR.MY_CAR: [{
     916: 8, 304: 8, 356: 4, 593: 8, 688: 5, 897: 8, 1265: 4, 854: 7, 870: 7, 871: 8, 354: 3, 872: 8, 608: 8, 320: 8, 339: 8, 544: 8, 809: 8, 832: 8, 909: 8, 1342: 6, 902: 8, 1107: 5, 1136: 8, 1151: 6, 1168: 7, 1173: 8, 1379: 8, 1040: 8, 1042: 8, 1078: 4, 127: 8, 1225: 8, 1227: 8, 1294: 8, 1312: 8, 1345: 8, 1363: 8, 1369: 8, 1384: 8, 1394: 8, 1407: 8, 1292: 8, 1170: 8, 1322: 8, 1287: 4, 1280: 1, 1155: 8, 1456: 4, 1427: 6, 1191: 2, 67: 8
   }]
}
#Todo: Get a byd VDS to see how fw could be queried. Currently added just for preventing ruffs error.

#FW_VERSIONS: dict[str, dict[tuple, list[bytes]]] = {}

FW_VERSIONS = {
  CAR.MY_CAR: {
    (Ecu.eps, 0x366, None): [  #：电动助力转向系统
      b'DUMMYDATA',
    ],
  },
}
