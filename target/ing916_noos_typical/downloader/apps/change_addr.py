# return abort_flag
def on_start_run(batch_counter: int):
    return False

# return abort_flag, new_data
def on_start_bin(batch_counter: int, bin_index: int, data: bytes):
    if bin_index != 2:
        return False, data
    ba = bytearray(data)
    ba[0] = batch_counter
    return False, bytes(ba)
