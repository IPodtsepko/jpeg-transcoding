import torch
import torch.nn as nn

__all__ = ["QECNN"]

old_mode = False

def _preserving_dimension_conv(
    in_channels: int, out_channels: int, kernel_size: int, add_activation: bool = True
) -> nn.Module:
    """
    Create a convolutional layer with PReLU activation. Sizes of the input and
    output tensors for this layer are the same.

    Parameters
    ----------
    in_channels : int
        Number of input channels.
    out_channels : int
        Number of output channels.
    kernel_size: int
        Size of the convolutional kernel.

    Returns
    -------
    Sequential module containing Conv2d and PReLU layers.
    """
    padding = (kernel_size - 1) // 2
    convolution = nn.Conv2d(
        in_channels=in_channels,
        out_channels=out_channels,
        kernel_size=kernel_size,
        padding=padding,
    )

    if old_mode or add_activation:
        return nn.Sequential(
            convolution,
            nn.PReLU(num_parameters=out_channels if not old_mode else 1),
        )
    return convolution


class QECNN(nn.Module):
    def __init__(self):
        """
        Initializes the QE-CNN model.
        """
        super(QECNN, self).__init__()

        self.conv11 = _preserving_dimension_conv(
            in_channels=3, out_channels=128, kernel_size=9
        )
        self.conv12 = _preserving_dimension_conv(
            in_channels=128, out_channels=64, kernel_size=7
        )
        self.conv13 = _preserving_dimension_conv(
            in_channels=64, out_channels=64, kernel_size=3
        )
        self.conv14 = _preserving_dimension_conv(
            in_channels=64, out_channels=32, kernel_size=1
        )

        self.conv21 = _preserving_dimension_conv(
            in_channels=3, out_channels=128, kernel_size=9
        )
        self.conv22 = _preserving_dimension_conv(
            in_channels=256, out_channels=64, kernel_size=7
        )
        self.conv23 = _preserving_dimension_conv(
            in_channels=128, out_channels=64, kernel_size=3
        )
        self.conv24 = _preserving_dimension_conv(
            in_channels=128, out_channels=32, kernel_size=1
        )

        self.conv5 = _preserving_dimension_conv(
            in_channels=64, out_channels=3, kernel_size=5, add_activation=False
        )

    def forward(self, x):
        """
        Forward pass of the QualityEnhancementCNN model.

        :param x: Input tensor.
        :return: Output tensor after processing through convolutional layers.
        """
        a = self.conv11(x)
        y = self.conv21(x)
        y = torch.cat([a, y], dim=1)

        a = self.conv12(a)
        y = self.conv22(y)
        y = torch.cat([a, y], dim=1)

        a = self.conv13(a)
        y = self.conv23(y)
        y = torch.cat([a, y], dim=1)

        a = self.conv14(a)
        y = self.conv24(y)
        y = torch.cat([a, y], dim=1)

        y = self.conv5(y)

        return x + y
