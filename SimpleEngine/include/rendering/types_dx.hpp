#pragma once

namespace SE
{
enum QueueType
{
    DirectQueue,
    ComputeQueue,
    CopyQueue
};

enum ChannelSize
{
    Unknown,
    SingleChannel,
    DoubleChannel,
    TripleChannel,
    FourChannel
};

enum FillMode
{
    WireFrame,
    Solid
};

enum CullMode
{
    None,
    Front,
    Back
};

enum Topology
{
    Undefined,
    Point,
    Line,
    Triangle,
    Patch
};

enum ComparisonFunction
{
    NoComparison,
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum DepthMask
{
    All,
    NoMask,
};

enum StencilOperation
{
    Keep,
    ToZero,
    Replace,
    Invert,
    Increment,
    Decrement
};

enum BlendOperation
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

// dst -> destination
// src -> source
enum BlendMode
{
    Zero,
    One,
    SrcColor,
    InverseSrcColor,
    SrcAlpha,
    InverseSrcAlpha,
    DstAlpha,
    InverseDstAlpha,
    SrcAlphaSat
};

enum BlendMask
{
    Red = 1,
    Green = 1 << 1,
    Blue = 1 << 2,
    Alpha = 1 << 3,
    AllChannels = Red | Green | Blue | Alpha
};

enum LogicOperation
{
    Clear,
    Set,
    Copy,
    CopyInv,
    Inverting,
    And,
    Nand,
    Or,
    Nor,
    Xor,
    AndReverse,
    AndInverted,
    OrReversed,
    OrInverted
};
}  // namespace SE