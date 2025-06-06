@0x855494f355dd77fb;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("seam::schema");

struct Vector2 {
    x @0 :Float32;
    y @1 :Float32;
}

struct PinOut {
    name        @0 :Text;
    id          @1 :UInt64;
    type        @2 :UInt16;
    children    @3 :List(PinOut);
    numCoords   @4 :UInt16;
}

struct PinValue {
    union {
        intValue    @0 :Int32;
        uintValue   @1 :UInt32;
        floatValue  @2 :Float32;
        stringValue @3 :Text;
        boolValue   @4 :Bool;
    }
}

struct PinIn {
    name        @0 :Text;
    id          @1 :UInt64;
    values      @2 :List(PinValue);
    type        @3 :UInt16;
    children    @4 :List(PinIn);
    numCoords   @5 :UInt16;
}

struct Property {
    name    @0 :Text;
    values  @1 :List(PinValue);
}

struct Node {
    position    @0 :Vector2;
    displayName @1 :Text;
    nodeName    @2 :Text;
    id          @3 :UInt64;
    
    inputPins   @4 :List(PinIn);
    outputPins  @5 :List(PinOut);
    properties  @6 :List(Property);
}

struct PinConnection {
    outId   @0 :UInt64;
    inId    @1 :UInt64;
}

struct NodeGraph {
    nodes               @0 :List(Node);
    connections         @1 :List(PinConnection);
    name                @2 :Text;
    maxNodeId           @3 :UInt64;
    maxPinId            @4 :UInt64;
    visualOutputNodeId  @5 :UInt64;
}