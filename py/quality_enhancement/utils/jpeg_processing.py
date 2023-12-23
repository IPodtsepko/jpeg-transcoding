import torch

from torch_dct import dct_2d, idct_2d
from .converters import get_luminance_from_rgb, to_yuv, to_rgb

__all__ = ["dct", "idct", "combine_images_with_masks"]


def _transform_blocks(
    function: callable, components: torch.Tensor, *args, **kwargs
) -> torch.Tensor:
    _, height, width = components.shape
    for i in range(0, height, 8):
        for j in range(0, width, 8):
            block_indexes = slice(None), slice(i, i + 8), slice(j, j + 8)
            components[block_indexes] = function(
                components[block_indexes], *args, **kwargs
            )
    return components


def dct(components: torch.Tensor) -> torch.Tensor:
    return _transform_blocks(dct_2d, components, norm="ortho")


def idct(components: torch.Tensor) -> torch.Tensor:
    return _transform_blocks(idct_2d, components, norm="ortho")


def _combine_blocks_with_mask(
    original_block: torch.Tensor, predicted_block: torch.Tensor, mask: list[bool]
) -> None:
    ZIGZAG_ORDER = [
        [0, 1, 5, 6, 14, 15, 27, 28],
        [2, 4, 7, 13, 16, 26, 29, 42],
        [3, 8, 12, 17, 25, 30, 41, 43],
        [9, 11, 18, 24, 31, 40, 44, 53],
        [10, 19, 23, 32, 39, 45, 52, 54],
        [20, 22, 33, 38, 46, 51, 55, 60],
        [21, 34, 37, 47, 50, 56, 59, 61],
        [35, 36, 48, 49, 57, 58, 62, 63],
    ]
    for i in range(8):
        for j in range(8):
            if mask[ZIGZAG_ORDER[i][j]]:
                predicted_block[:, i, j] = original_block[:, i, j]


def _combine_ys_with_masks(
    original_dct: torch.Tensor,
    predicted_dct: torch.Tensor,
    masks: list[list[bool]],
    sampling: int,
    block_id: int = 0,
):
    block_size = 8 * sampling
    _, height, width = original_dct.shape

    for i in range(0, height, block_size):
        for j in range(0, width, block_size):
            block_indexes = (
                slice(None),
                slice(i, i + block_size),
                slice(j, j + block_size),
            )
            original_block = original_dct[block_indexes]
            predicted_block = predicted_dct[block_indexes]
            if sampling != 1:
                _combine_ys_with_masks(
                    original_block, predicted_block, masks, sampling, block_id
                )
            else:  # block 8x8
                mask = masks[block_id % len(masks)]
                _combine_blocks_with_mask(original_block, predicted_block, mask)

            block_id += sampling**2


def get_dct_of_luminance_from_rgb(images):
    return dct(get_luminance_from_rgb(images))


def combine_images_with_masks(
    original_images: torch.Tensor,
    predicted_images: torch.Tensor,
    masks: list[list[bool]],
    sampling: torch.Tensor,
) -> torch.Tensor:
    original_dct = dct(get_luminance_from_rgb(original_images, scaled=True))
    predicted_yuv = to_yuv(predicted_images * 255.0)
    dct(predicted_yuv[:, :, :, 0])
    for i in range(len(sampling)):
        _combine_ys_with_masks(
            original_dct=original_dct[i],
            predicted_dct=predicted_yuv[i, :, :, 0],
            masks=masks,
            sampling=1 if sampling[i] == 0 else 2,
        )
    idct(predicted_yuv[:, :, :, 0])
    return to_rgb(predicted_yuv)
