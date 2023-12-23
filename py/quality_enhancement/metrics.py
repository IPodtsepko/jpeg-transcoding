import torch
import torch.nn as nn

from .utils.converters import get_luminance_from_rgb
from .utils.jpeg_processing import dct

__all__ = ["YMSELoss", "DCTLoss", "get_metric"]


def _psnr(mse: float, max: float = 1.0):
    return 10 * torch.log10(max**2 / mse)


class YMSELoss(nn.Module):
    def __init__(self):
        super(YMSELoss, self).__init__()

    def forward(self, inputs, targets):
        return YMSELoss.evaluate(inputs, targets)

    @staticmethod
    def psnr(mse: float) -> float:
        return _psnr(mse)

    @staticmethod
    def evaluate(inputs, targets, psnr: bool = False):
        if inputs.size() != targets.size():
            raise ValueError("Input and target must have the same size.")

        inputs_ys = get_luminance_from_rgb(inputs, scaled=True)
        targets_ys = get_luminance_from_rgb(targets, scaled=True)

        mse = ((inputs_ys - targets_ys) ** 2).mean()
        if not psnr:
            return mse

        return YMSELoss.psnr(mse)


class DCTLoss(nn.Module):
    def __init__(self):
        super(DCTLoss, self).__init__()

    def forward(self, inputs, targets):
        return DCTLoss.evaluate(inputs, targets, psnr=False)

    @staticmethod
    def psnr(mse):
        max_dct_value = 1016.0  # white image
        min_dct_value = -1024.0  # black image
        return _psnr(mse, max=(max_dct_value - min_dct_value))

    @staticmethod
    def evaluate(inputs, targets, psnr: bool = False):
        if inputs.size() != targets.size():
            raise ValueError("Input and target must have the same size.")

        inputs_ys = get_luminance_from_rgb(inputs, scaled=True)
        targets_ys = get_luminance_from_rgb(targets, scaled=True)

        mse = ((dct(inputs_ys) - dct(targets_ys)) ** 2).mean()
        return mse if not psnr else DCTLoss.psnr(mse)


def get_metric(name: str) -> nn.Module:
    match name:
        case "YMSE":
            return YMSELoss()
        case "DCT":
            return DCTLoss()
        case _:
            return nn.MSELoss()
