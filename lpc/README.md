# LPC11Cxx programmer via CAN BUS.

Node ID = 0x7d (125)

## AAA

Request Server -> Client  (0x600 + Node ID):

| Can Header   | rtr | len | Data 0  | Data 1 | Data 2 | Data 3     | Data 4  | Data 5  | Data 6  | Data 7  |
| ------------ | --- | --- | ------- | --------------- |----------- | ------- | ------- | ------- | ------- |
| 0x600 + node | 0   | 8   | Command | Index           | Sub Index  | Data                                  |

Response, Client -> Server (0x580 + Node ID):

| Can Header   | rtr | len | Data 0  | Data 1 | Data 2 | Data 3     | Data 4  | Data 5  | Data 6  | Data 7  |
| ------------ | --- | --- | ------- | --------------- |----------- | ------- | ------- | ------- | ------- |
| 0x580 + node | 0   | 8   | Command | Index           | Sub Index  | Data                                  |

Operations:

| Command code | Operation                     |
| ------------ | ----------------------------- |
| 0x40         | Read Dictionary Object (RDO)  |
| ------------ | ----------------------------- |
| 0x43         | Read Dictionary Object reply, expedited, 4 bytes sent |
| 0x47         | Read Dictionary Object reply, expedited, 3 bytes sent |
| 0x4B         | Read Dictionary Object reply, expedited, 2 bytes sent |
| 0x4F         | Read Dictionary Object reply, expedited, 1 bytes sent |
| ------------ | ----------------------------- |
| 0x23         | Write Dictionary Object reply, expedited, 4 bytes sent |
| 0x27         | Write Dictionary Object reply, expedited, 3 bytes sent |
| 0x2B         | Write Dictionary Object reply, expedited, 2 bytes sent |
| 0x2F         | Write Dictionary Object reply, expedited, 1 bytes sent |

## Example 

In PCAN-View we transmitted the following message one time:

```
ID = 0x67D Len = 8 Data = 0x40 0x00 0x10 0x00 0x00 0x00 0x00 0x00
```

The bootloader responded with the following message:

```
ID = 0x5FD Len = 8 Data = 0x43 0x00 0x10 0x00 0x4C 0x50 0x43 0x31
```
