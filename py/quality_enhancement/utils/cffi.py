from cffi import FFI

__all__ = ["LibUtils"]

class LibUtils:
    def __init__(self):
        self._ffi = FFI()
        self._lib = self._ffi.dlopen("../build/libUtils.dylib")
        self._ffi.cdef(
            """
            size_t get_dct_filter_masks_count(const size_t power);
            size_t * get_dct_filter_masks(const size_t power);
            void free_masks(size_t * masks);
            """
        )

    def get_masks(self, power: int) -> list[list[bool]]:
        n = self._lib.get_dct_filter_masks_count(power)
        masks = self._lib.get_dct_filter_masks(power)
        result = [
            [x == "1" for x in reversed(format(masks[i], "064b"))] for i in range(n)
        ]
        self._lib.free_masks(masks)
        return result
