// Parametric model generated from 3MF mesh
// Generated: Automated conversion tool

// === PARAMETERS ===
// Main dimensions
overall_width = 48.00;
overall_depth = 41.65;
overall_height = 43.66;

// Bounding box reference
min_x = -24.00;
min_y = -20.82;
min_z = -21.83;

// Rendering quality
$fn = 50;  // Increase for smoother circles

// === MODEL ===
module main_body() {
    // Base rectangular body
    translate([0.00, 0.00, 0.00])
        cube([overall_width, overall_depth, overall_height], center=true);
}


// === ASSEMBLY ===
union() {
    main_body();
}
