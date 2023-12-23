import torch

__all__ = ["to_yuv", "to_rgb", "get_luminance_from_uyv", "get_luminance_from_rgb"]

_rgb_to_yuv = torch.tensor(
    [
        [+0.29900, -0.16874, +0.50000],
        [+0.58700, -0.33126, -0.41869],
        [+0.11400, +0.50000, -0.08131],
    ]
)

_yuv_to_rgb = torch.inverse(_rgb_to_yuv)


def _check_shape_for_color_space_conversion(
    images: torch.Tensor, color_index: int = -1
) -> None:
    shape = images.shape
    assert len(shape) == 4 and shape[color_index] == 3, f"Invalid tensor shape: {shape}"


def to_yuv(images: torch.Tensor, scaled: bool = False) -> torch.Tensor:
    _check_shape_for_color_space_conversion(images, color_index=1)
    result = images.permute(0, 2, 3, 1).float() @ _rgb_to_yuv.to(images.device)
    factor = 255.0 if scaled else 1.0
    result[:, :, :, 0] -= 128.0 / factor
    return result


def to_rgb(images: torch.Tensor) -> torch.Tensor:
    _check_shape_for_color_space_conversion(images)
    result = images.clone()
    result[:, :, :, 0] += 128.0
    return (
        torch.clip(torch.round(result @ _yuv_to_rgb), 0.0, 255.0)
        .permute(0, 3, 1, 2)
        .int()
    )

def get_luminance_from_uyv(images: torch.Tensor) -> torch.Tensor:
    return images[:, :, :, 0]

def get_luminance_from_rgb(images: torch.Tensor, scaled: bool = False) -> torch.Tensor:
    return get_luminance_from_uyv(to_yuv(images, scaled))
