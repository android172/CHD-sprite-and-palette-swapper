#include "opd.h"

#include "file_utils.h"
#include "util.h"

#include <QDebug>

// Constructor & Destructor
Opd::Opd(
    const QLinkedList<GFXPage>&   gfx_pages,
    const QLinkedList<Sprite>&    sprites,
    const QLinkedList<Frame>&     frames,
    const QLinkedList<Animation>& animations,
    const QLinkedList<Palette>&   palettes
)
    : gfx_pages(gfx_pages), sprites(sprites), frames(frames),
      animations(animations), palettes(palettes) {}
Opd::~Opd() {}

// ////////////////// //
// OPD PUBLIC METHODS //
// ////////////////// //

// Helper functions
QLinkedList<Palette> read_palettes(QString const& path);

Opd* Opd::open(const QString& path) {
    const QString file_name = path.split('/').last().toLower();
    const QString file_dir  = path.chopped(file_name.size());
    std::ifstream opd_file { path.toStdString(),
                             std::ios::binary | std::ios::in };
    if (!opd_file.is_open()) throw std::runtime_error("File not found.");

    // === Sections ===
    opd_file.seekg(0x24);
    const auto gibberish_offs  = read_type<uint>(opd_file);
    const auto gibberish_size  = read_type<uint>(opd_file);
    const auto palette_offs    = read_type<uint>(opd_file);
    const auto palette_size    = read_type<uint>(opd_file);
    const auto gfx_pages_offs  = read_type<uint>(opd_file);
    const auto gfx_pages_size  = read_type<uint>(opd_file);
    const auto frames_offs     = read_type<uint>(opd_file);
    const auto frames_size     = read_type<uint>(opd_file);
    const auto animations_offs = read_type<uint>(opd_file);
    const auto animations_size = read_type<uint>(opd_file);

    // === Gibberish section (SKIP) ===
    // opd_file.seekg(gibberish_offs);

    // === Palette section ===
    // Read name
    opd_file.seekg(palette_offs + 5);
    const auto palette_name = read_type<QString>(opd_file).toLower();
    const auto palette_path = file_dir + palette_name;

    // Read palettes
    auto palettes = read_palettes(palette_path);

    // === GFX page section ===
    // Parse header
    opd_file.seekg(gfx_pages_offs);
    const auto gfx_count = read_type<ushort>(opd_file);
    opd_file.seekg(gfx_pages_offs + 16);

    // Read pages
    QLinkedList<GFXPage> gfx_pages {};
    for (auto i = 0; i < gfx_count; i++) {
        GFXPage gfx {};
        gfx.index  = i;
        gfx.name   = read_type<QString>(opd_file);
        gfx.dir    = file_dir;
        gfx.width  = read_type<ushort>(opd_file);
        gfx.height = read_type<ushort>(opd_file);
        read_type<uchar>(opd_file); // Unknown
        gfx.initialize();
        gfx_pages.push_back(gfx);
    }

    // === Frame section ===
    // Parse header
    opd_file.seekg(frames_offs);
    const auto frame_count = read_type<ushort>(opd_file);
    opd_file.seekg(frames_offs + 16);

    // Keeping track of all sprites
    QLinkedList<Sprite> sprites {};

    // Read frames
    QLinkedList<Frame> frames {};
    for (auto i = 0; i < frame_count; i++) {
        Frame frame {};
        frame.index    = i;
        frame.name     = read_type<QString>(opd_file);
        frame.x_offset = read_type<short>(opd_file);
        frame.y_offset = read_type<short>(opd_file);
        read_type<ushort>(opd_file); // Unknown

        // Read counts
        const auto hit_box_count = read_type<ushort>(opd_file);
        const auto part_count    = read_type<ushort>(opd_file);
        read_type<ushort>(opd_file); // Unknown

        // Read hit boxes
        for (auto i = 0; i < hit_box_count; i++) {
            Frame::HitBox hit_box {};
            hit_box.index      = i;
            hit_box.x_position = read_type<ushort>(opd_file);
            hit_box.y_position = read_type<ushort>(opd_file);
            hit_box.width      = read_type<ushort>(opd_file);
            hit_box.height     = read_type<ushort>(opd_file);
            frame.hit_boxes.push_back(hit_box);
        }

        // Read parts
        for (auto i = 0; i < part_count; i++) {
            Frame::Part part {};
            part.index    = i;
            part.x_offset = read_type<short>(opd_file);
            part.y_offset = read_type<short>(opd_file);

            // Read sprite
            Sprite sprite {};
            sprite.gfx_page = get_it_at(gfx_pages, read_type<ushort>(opd_file));
            sprite.gfx_x_pos = read_type<ushort>(opd_file);
            sprite.gfx_y_pos = read_type<ushort>(opd_file);
            sprite.width     = read_type<ushort>(opd_file);
            sprite.height    = read_type<ushort>(opd_file);

            // Set sprite
            auto sprite_i = std::find(sprites.begin(), sprites.end(), sprite);
            if (sprite_i == sprites.end()) {
                sprite.index = sprites.size();
                sprite.initialize();
                sprites.push_back(sprite);
                sprite_i = sprites.end() - 1;
            }
            part.sprite = sprite_i;

            // Read rest
            part.palette   = get_it_at(palettes, read_type<uchar>(opd_file));
            part.flip_mode = read_type<uchar>(opd_file);
            // Unknown
            read_type<ushort>(opd_file);
            read_type<ushort>(opd_file);
            read_type<ushort>(opd_file);
            frame.parts.push_back(part);
        }
        frame.initialize();
        frames.push_back(frame);
    }

    // === Animation section ===
    // Parse header
    opd_file.seekg(animations_offs);
    const auto animation_count = read_type<ushort>(opd_file);
    opd_file.seekg(animations_offs + 16);

    // Read animations
    QLinkedList<Animation> animations {};
    for (auto i = 0; i < animation_count; i++) {
        Animation animation {};
        animation.index = i;
        animation.name  = read_type<QString>(opd_file);

        // Read count
        const auto anim_frame_count = read_type<ushort>(opd_file);
        read_type<uint>(opd_file); // Unknown

        // Read frames
        for (auto j = 0; j < anim_frame_count; j++) {
            Animation::Frame frame {};
            frame.index = j;
            frame.data  = get_it_at(frames, read_type<ushort>(opd_file));
            frame.delay = read_type<ushort>(opd_file);
            read_type<uchar>(opd_file); // Unknown
            frame.x_offset = read_type<short>(opd_file);
            frame.y_offset = read_type<short>(opd_file);
            read_type<uint>(opd_file); // Unknown
            read_type<uint>(opd_file); // Unknown
            read_type<uint>(opd_file); // Unknown

            // Mark frame as used
            frame.data->used = true;

            animation.frames.push_back(frame);
        }
        animations.push_back(animation);
    }

    opd_file.close();

    return new Opd(gfx_pages, sprites, frames, animations, palettes);
}

// //////////////////// //
// OPD HELPER FUNCTIONS //
// //////////////////// //

QLinkedList<Palette> read_palettes(QString const& path) {
    // Load palettes
    std::ifstream col_file { path.toStdString(),
                             std::ios::in | std::ios::binary };
    if (!col_file.is_open())
        throw std::runtime_error(
            "Couldn't load palette file from path: \"" + path.toStdString() +
            "\"."
        );
    col_file.seekg(0x1F);

    // Compute multiplier
    uchar const col_multiplier = std::max(4U * read_type<uchar>(col_file), 1U);

    // Compute palettes
    QLinkedList<Palette> palettes {};
    for (auto i = 0; i < 256; i++) {
        Palette palette {};
        palette.index = i;
        for (auto& color : palette) {
            // read palette color
            uchar b = col_multiplier * read_type<uchar>(col_file);
            uchar g = col_multiplier * read_type<uchar>(col_file);
            uchar r = col_multiplier * read_type<uchar>(col_file);
            color   = { r, g, b };
            // read separation byte
            read_type<uchar>(col_file);
        }
        palettes.push_back(palette);
    }

    col_file.close();
    return palettes;
}