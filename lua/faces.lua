local faces = {}

for i = 0, 5 do
    faces[i] = {
        pattern = 0,
        photoPath = nil
    }
end

local function clampIndex(index)
    if index < 0 then return 0 end
    if index > 5 then return 5 end
    return index
end

function toggleFacePattern(faceIndex)
    local idx = clampIndex(faceIndex)
    local face = faces[idx]
    if not face then
        return 0
    end
    face.pattern = (face.pattern + 1) % 3
    return face.pattern
end

function getFacePattern(faceIndex)
    local idx = clampIndex(faceIndex)
    local face = faces[idx]
    if not face then
        return 0
    end
    return face.pattern
end

function setFacePhoto(faceIndex, path)
    local idx = clampIndex(faceIndex)
    local face = faces[idx]
    if not face then
        return
    end
    face.photoPath = path
end

function getFacePhoto(faceIndex)
    local idx = clampIndex(faceIndex)
    local face = faces[idx]
    if not face then
        return nil
    end
    return face.photoPath
end

