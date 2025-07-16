use <utils.scad>;
use <screws.scad>;

// constants
$e = 0.01; // small extra for removing artifacts during differencing
$2e = 2 * $e; // twice epsilon for extra length when translating by -e
$fn = $preview ? 10 : 60; // number of default vertices when rendering cylinders
nylon_tol = 0.1; // extra tolerance for carbon fibre nylon parts shrinkage
tglase_tol = 0.025; // extra tolerance for tglase parts

// sizes
//$extra_tolerance = 2 * nylon_tol; // for cutouts, screws, etc.
$extra_tolerance = 2 * tglase_tol; // for cutouts, screws, etc.
$extra_tolerance_2D = [$extra_tolerance, $extra_tolerance]; // tolerance in 2 d
$screw_hole_tolerance = 0.45 + $extra_tolerance; // tolerance for screw holes
$screw_threadable_tolerance = 0; // tolerance for screw holes that can be threaded into

// controller box
// depth:
//  - display recess: 1.0 mm
//  - display + board height: 4.0mm
//  - standoff: 28.0 mm
//  - pcb: 1.6mm
//  - standoffs: 12mm (has to be M3, M2.5 doesn't have this length)
//  - back wall: 4.0 mm
//  total: 50.6
//  - barrel jack: 12.4mm from board to wall
// --> indent the plug by 0.4 mm
//  - RJ45: 16.2mm from board to plug --> should work well with 12mm standoff

$box_size = [85, 58, 50.6]; //$box_size = [85, 58, 50];
$box_wall_thickness = 2; // thickness of the box walls
$box_front_thickness = 4; // front thickness
$box_corner_radius = 3; // radius of the corners
$box_back_thickness = $box_front_thickness; // thickness of the back wall
$box_back_wall_gap = 0.3 + $extra_tolerance; // gap between the back insent and the walls
$box_back_attachment_screws_indent = 4; // indentation from the edge
$box_back_attachment_screws_gap = 0.1; // gap in the screw sockets

// oled
$oled_screen = [57.3, 29.3] + 2 * $extra_tolerance_2D;
$oled_screen_offset = [0, -2.2]; // offset from center of board
$oled_display = [62.5, 40.0] + 2 * $extra_tolerance_2D;
$oled_display_recessed = 1.0; // recess from screen
$oled_display_offset = [0, 1.2]; // offset from center of board
$oled_board = [72.5, 45.5]; // oled board size (plus space for screws)
$oled_board_recessed = 0.5; // recess from display

// magnet adapters
$magnet_adapter_width = 4; // adapter width
$magnet_adapter_depth = 4;
$magnet_adapter_corner_radius = 0.5;
$magnet_adapter_tolerance = 0.2; // tolerance between adapter and insert
$magnet_adapter_height_gap = 1.0; // how much space to leave in the adapter height
$magnet_diameter = 25.4 + 3 * $extra_tolerance; // this is the 1" diameter
$magnet_thickness_0_1 = 2.54; // this is the 1/10" thick
$magnet_thickness_0_125 = 3.18; // this is the 1/8" thick
$magnet_wall_thickness = 3; // how thick are the walls around the magnets
$magnet_base_thickness = 0.5; // how thick is the base of the magnet holder
$magnet_distance = 35; // distance between magnets

// pcb, power, ethernet, antenna
$pcb_screws_position = [28, 19]; // x/y coordinates relative to center
$pcb_screws_offset = 0.75; // offset from the back wall
$power_jack_position = [-20.5, -2.5]; // x/y coordinates relative to center
$power_jack_thread_diameter = 8.1 + 0.2 + 2 * $extra_tolerance;
$power_jack_support_diameter = 11.3 + 0.2 + 2 * $extra_tolerance;
$power_jack_support_offset = $box_back_thickness - 2.0; // offset from back wall
$ethernet_position = [2, -1.47]; // x/y coordinates relative to center
$ethernet_cutout = [15.5, 15.9] + [0.2, 0.2] + 2 * $extra_tolerance_2D;

// antenna best placed inside housing (on back wall) and design of P2 antenna not conducive to going through a cutout
//$antenna_position = [$box_size[0]/2 - 1.5 - $box_back_wall_gap/2 - $box_wall_thickness, 12];
//$antenna_cutout = [3, 9];

// controller back
module module_controller_back() {
  difference() {
    union() {
      // back wall
      linear_extrude(height = $box_wall_thickness)
        offset(r = $box_corner_radius)
          offset(delta = -$box_corner_radius)
            square([$box_size[0], $box_size[1]], center = true);
      // elevated back center
      x = $box_size[0] - 2 * $box_wall_thickness - 2 * $box_back_wall_gap;
      y = $box_size[1] - 2 * $box_wall_thickness - 2 * $box_back_wall_gap;
      edge = 2 * $box_back_attachment_screws_indent;
      linear_extrude(height = $box_back_thickness)
        polygon([
            [x/2 - edge, y/2], [x/2, y/2 - edge],
            [x/2, -y/2 + edge], [x/2 - edge, -y/2],
            [-x/2 + edge, -y/2], [-x/2, -y/2 + edge],
            [-x/2, y/2 - edge], [-x/2 + edge, y/2]
          ]);

    }

    // back wall attachments screws
    for (x = [-1, 1]) {
      for (y = [-1, 1]) {
        translate([-x * ($box_size[0]/2 - $box_back_attachment_screws_indent), y * ($box_size[1]/2 - $box_back_attachment_screws_indent), -$e])
          machine_screw(name = "M2.5", length = $box_back_thickness + $2e, tolerance = $screw_hole_tolerance);
      }
    }

    // power jack cutout
    translate([$power_jack_position[0], $power_jack_position[1], -$e])
      cylinder(d = $power_jack_thread_diameter, h = $box_back_thickness + $2e);
    translate([$power_jack_position[0], $power_jack_position[1], $power_jack_support_offset])
      cylinder(d = $power_jack_support_diameter, h = $box_back_thickness);

    // ethernet cutout
    translate([$ethernet_position[0], $ethernet_position[1], -$e])
      xy_center_cube([$ethernet_cutout[0], $ethernet_cutout[1], $box_back_thickness + $2e]);

    // antenna cutout
    /*
    translate([$antenna_position[0], $antenna_position[1], -$e])
      xy_center_cube([$antenna_cutout[0], $antenna_cutout[1], $box_back_thickness + $2e]);
    */

    // pcb support screws
    for (x = [-1, 1]) {
      for (y = [-1, 1]) {
        translate([x * $pcb_screws_position[0], y * $pcb_screws_position[1], -$e])
          machine_screw(name = "M2.5", length = $box_back_thickness + $2e, tolerance = $screw_hole_tolerance);
      }
    }

  }
}

// controller front
module module_controller_front() {
  // construct the front
  difference() {
    union() {
      // box walls
      difference() {
        // box with rounded edges
        linear_extrude(height = $box_size[2] - $box_wall_thickness)
          offset(r = $box_corner_radius)
            offset(delta = -$box_corner_radius)
              square([$box_size[0], $box_size[1]], center = true);
        // inside cutout
        translate([0, 0, $box_front_thickness])
          linear_extrude(height = $box_size[2])
            offset(r = $box_corner_radius)
              offset(delta = -$box_corner_radius)
                square([$box_size[0] - 2 * $box_wall_thickness, $box_size[1] - 2 * $box_wall_thickness], center = true);
      }

      // magnet attachments long edge
      for (x = [-1, 1])
        translate([0, x * ($box_size[1]/2 - $magnet_adapter_depth/2 - $box_wall_thickness), $box_front_thickness])
        mirror([0, if (x == -1) 1 else 0, 0])
        module_controller_magnet_adapters(
          width = $box_size[0] - 2 * $box_wall_thickness - 2 * $magnet_adapter_depth,
          height = $box_size[2] - $box_front_thickness - $box_back_thickness - $extra_tolerance
        );

      // magnet attachments short edge
      for (y = [-1, 1])
        translate([y * ($box_size[0]/2 - $magnet_adapter_depth/2 - $box_wall_thickness), 0, $box_front_thickness])
        mirror([if (y == 1) 1 else 0, 0, 0])
        rotate([0, 0, 90])
        module_controller_magnet_adapters(
          width = $box_size[1] - 2 * $box_wall_thickness - 2 * $magnet_adapter_depth,
          height = $box_size[2] - $box_front_thickness - $box_back_thickness - $extra_tolerance
        );

      // corner pillars
      for (y = [-1, 1]) for (x = [-1, 1])
        translate([y * ($box_size[0]/2 - $magnet_adapter_depth/2 - $box_wall_thickness), x * ($box_size[1]/2 - $magnet_adapter_depth/2 - $box_wall_thickness), $box_front_thickness])
          xy_center_cube([$magnet_adapter_depth + $2e, $magnet_adapter_depth + $2e, $box_size[2] - $box_front_thickness - $box_back_thickness - $extra_tolerance]);
    }

    // back wall attachments screws
    for (x = [-1, 1]) {
      for (y = [-1, 1]) {
        translate([x * ($box_size[0]/2 - $box_back_attachment_screws_indent), y * ($box_size[1]/2 - $box_back_attachment_screws_indent), $box_front_thickness])
          machine_screw(name = "M2.5", length = $box_size[2] - $box_front_thickness - $box_back_thickness + $e, countersink = false, tolerance = $screw_threadable_tolerance);
      }
    }
    // attachment screw gaps (for better print paths)
    for (y = [-1, 1]) for (x = [-1, 1])
      translate([y * ($box_size[0]/2 - $magnet_adapter_depth - $box_wall_thickness), x * ($box_size[1]/2 - $magnet_adapter_depth - $box_wall_thickness), $box_front_thickness])
        rotate([0, 0, -y * x * 45])
          xy_center_cube([$box_back_attachment_screws_gap, $magnet_adapter_depth, $box_size[2] - $box_front_thickness - $box_back_thickness + $e]);

    // OLED board cutout
    translate([0, 0, $oled_display_recessed + $oled_board_recessed])
      xy_center_cube([$oled_board[0], $oled_board[1], $box_front_thickness]);

    // OLED display cutout
    translate([$oled_display_offset[0], $oled_display_offset[1], $oled_display_recessed])
      xy_center_cube([$oled_display[0], $oled_display[1], $box_front_thickness]);

    // OLED screen cutout
    translate([$oled_screen_offset[0], $oled_screen_offset[1], -$e])
      xy_center_cube([$oled_screen[0], $oled_screen[1], $box_front_thickness + $2e]);
  }
}

module module_controller_magnet_adapters(width, height) {
  translate([-width/2, -$magnet_adapter_depth/2, 0])
  linear_extrude(height = height)
    for (x = [0, 1])
      translate([x * width, 0]) mirror([x, 0, 0])
        union() {
            // bottom rounding
            difference() {
              offset(r = $magnet_adapter_corner_radius)
                offset(delta = -$magnet_adapter_corner_radius)
                polygon(points=[
                    [0, 0],
                    [$magnet_adapter_width, 0],
                    [0, $magnet_adapter_depth]
                  ]);
              square([$magnet_adapter_width/2, $magnet_adapter_depth]);
            }
            // top rounding
            difference() {
              square([$magnet_adapter_width/2, $magnet_adapter_depth]);
              offset(r = $magnet_adapter_corner_radius)
                offset(delta = -$magnet_adapter_corner_radius)
                polygon(points=[
                    [$magnet_adapter_width, $magnet_adapter_depth],
                    [$magnet_adapter_width, 0],
                    [0, $magnet_adapter_depth]
                  ]);
            }
        }
}

module module_controller_magnet_inserts(width, height, magnet_thickness, n_magnets = 2) {
  //translate([-width/2, -$magnet_adapter_depth/2, 0])
  tolerance = 2 * $magnet_adapter_tolerance;
  difference() {
    linear_extrude(height = height)
    union() {
        // adapter insert rounded wedge
        for (x = [0, 1])
          translate([x * width, 0]) mirror([x, 0, 0])
        offset(delta = -tolerance)
          offset(r = $magnet_adapter_corner_radius)
            offset(delta = -$magnet_adapter_corner_radius)
              polygon(points=[
                  [$magnet_adapter_width, $magnet_adapter_depth],
                  [$magnet_adapter_width, 0],
                  [0, $magnet_adapter_depth]
                ]);
        // adapter insert rest of wedge
        for (x = [0, 1])
          translate([x * width, 0]) mirror([x, 0, 0])
            polygon(points=[
                [$magnet_adapter_width + tolerance, $magnet_adapter_depth - tolerance],
                [$magnet_adapter_width + tolerance, 0],
                [$magnet_adapter_width/2 + tolerance, $magnet_adapter_depth/2 + tolerance/2],
                [$magnet_adapter_width/2 + tolerance, $magnet_adapter_depth - tolerance]
              ]);
        // insert body
        translate([$magnet_adapter_width + tolerance, 0])
          square([width - 2 * ($magnet_adapter_width + tolerance) + $e, $magnet_adapter_depth - tolerance]);
    }
    // magnet holders
    if (n_magnets > 0) {
      mag = [ for (i = [1 : 1 : n_magnets]) i ];
      for (m = mag) {
        translate([width/2 - (n_magnets - 1) * $magnet_distance/2 + (m - 1) * $magnet_distance, $magnet_adapter_depth - magnet_thickness - $magnet_adapter_tolerance + $e, height/2])
        rotate([-90, 0, 0])
          cylinder(d = $magnet_diameter, h = magnet_thickness + $e);
      }
    }
    // grips
    for (z = [0, 1])
      translate([width/2, $magnet_adapter_depth/2, z * (height - height/2 + $magnet_diameter/2 + $magnet_wall_thickness + $2e) -$e])
        xy_center_cube([width - 3 * $magnet_adapter_width - 2 * $magnet_wall_thickness, $magnet_adapter_depth + $2e, height/2 - $magnet_diameter/2 - $magnet_wall_thickness]);
    // bottom cutoff
    if ($magnet_adapter_depth - $magnet_adapter_tolerance - magnet_thickness - $magnet_base_thickness > 0) {
        translate([-$e, -$e, -$e])
        cube([width + $2e, $magnet_adapter_depth - $magnet_adapter_tolerance - magnet_thickness - $magnet_base_thickness + $e, height + $2e], center = false);
    }
  }
}

module module_controller_magnet_holder() {
  rotate([90, 0, 0])
  module_controller_magnet_inserts(
    width = $box_size[0] - 2 * $box_wall_thickness - 2 * $magnet_adapter_depth,
    height = $box_size[2] - $box_front_thickness - $box_back_thickness - $magnet_adapter_height_gap,
    magnet_thickness = $magnet_thickness_0_1,
    n_magnets = 2
  );
}

/*
// decided to go without side magnets but it does work
// although 1/8" (0_125) thickness recomended to be
// able to use strong enough magnets
module module_controller_manget_holder_sides() {
  rotate([90, 0, 0])
  module_controller_magnet_inserts(
    width = $box_size[1] - 2 * $box_wall_thickness - 2 * $magnet_adapter_depth,
    height = $box_size[2] - $box_front_thickness - $box_back_thickness - $magnet_adapter_height_gap,
    magnet_thickness = $magnet_thickness_0_1, // $magnet_thickness_0_125,
    n_magnets = 1
  );
}
*/

module_controller_front();
translate([0, 0, $box_size[2] + 5])
rotate([180, 0, 0])
module_controller_back();
!module_controller_magnet_holder();
