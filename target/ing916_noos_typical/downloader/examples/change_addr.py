# return abort_flag
def on_start_run(batch_counter: int):
    return False

# return abort_flag, new_data
def on_start_bin(batch_counter: int, bin_index: int, data: bytes):
    if bin_index != 6:
        return False, data
    ba = bytearray(data)
    addr = batch_counter.to_bytes(4, 'little')
    ba[1:5] = addr
    return False, bytes(ba)