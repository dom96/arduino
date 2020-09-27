const data = staticRead("hpm_from_start.dat")

const HPM_CMD_RESP_HEAD = 0x40
const HPM_MAX_RESP_SIZE = 8 + 8
const HPM_READ_PARTICLE_MEASURMENT_LEN = 5
const HPM_READ_PARTICLE_MEASURMENT_LEN_C = 5 + 8

type
  HPM_PACKET = enum
    HPM_HEAD_IDX,
    HPM_LEN_IDX,
    HPM_CMD_IDX,
    HPM_DATA_START_IDX

  Packet = object
    head: byte
    len: uint8
    cmd: byte
    pm1: uint16
    pm2_5: uint16
    pm4_0: uint16
    pm10: uint16
    reserved1: byte
    reserved2: byte


var i = 0
while i < data.len:
  case data[i]
  of '\x40':
    var respBuf: array[13, byte]
    respBuf[HPM_HEAD_IDX.ord] = data[i].byte
    i.inc
    respBuf[HPM_LEN_IDX.ord] = data[i].byte
    i.inc
    echo("Got len: ", respBuf[HPM_LEN_IDX.ord])
    if respBuf[HPM_LEN_IDX.ord].int > HPM_MAX_RESP_SIZE:
      echo("Skipping, too long")
      continue

    respBuf[HPM_CMD_IDX.ord] = data[i].byte
    i.inc
    for j in 1 .. respBuf[HPM_LEN_IDX.ord].int:
      respBuf[HPM_CMD_IDX.ord + j] = data[i].byte
      i.inc
    echo(respBuf)
    var pk = cast[Packet](respBuf)
    echo(pk)
    echo("2.5 ", respBuf[HPM_CMD_IDX.ord + 2].int * 256 + respBuf[HPM_CMD_IDX.ord + 3].int)
    echo("10 ", respBuf[HPM_CMD_IDX.ord + 6].int * 256 + respBuf[HPM_CMD_IDX.ord + 7].int)
    break
  else:
    i.inc()