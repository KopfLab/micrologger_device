// common convenience functions and modules
use <screws.scad>
$e = 0.01; // small extra for removing artifacts during differencing
$2e= 0.02; // twice epsilon for extra length when translating by -e

// x/y but not z centered cube (regular center=true also centered z)
module xy_center_cube (size) {
  translate([-size[0]/2, -size[1]/2, 0])
  cube(size, center=false);
}

xy_center_cube([40, 20, 4]);

// rounded cube that can be flared
module rounded_cube (size, round_left = true, round_right = true, scale_y = 1.0, scale_x = 1.0, z_center = false) {
  move = (z_center) ? [0, 0, 0] : [0, 0, size[2]/2];
  translate(move)
  linear_extrude(height = size[2], center = true, convexity = 10, slices = 20, scale = [scale_x, scale_y])
  union() {
    rounds = (round_left && round_right) ? [-1, 1] : ((round_left) ? [-1] : ((round_right) ? [+1] : []));
    nonrounds = (!round_left && !round_right) ? [-1, 1] : ((!round_left) ? [-1] : ((!round_right) ? [+1] : []));
    for (x = rounds) translate([x * (size[0] - size[1])/2, 0]) circle(d = size[1]);
    for (x = nonrounds) translate([x * (size[0] - size[1])/2, 0]) square(size[1], center = true);
    square([size[0] - size[1], size[1]], center = true);
  }
}

translate([0, 80, 0]) color("aqua")
rounded_cube([30, 10, 20], round_left = true, round_right = true, scale_y = 0.5, scale_x = 0.25);

// x/y center cube with feet
// @param feet how many feet to add
// @param foot_height what the height of each foot is (in mm)
// @param tolerance what tolerance to build into the feet to make sure stacking works
// @param stackable whether it should be stackable or not
module xy_center_cube_with_feet (size, feet = 2, foot_height = 5, tolerance = 0.3, stackable = true) {
  foot_d = foot_height/cos(180/6);
  foot_list = [for (i = [1 : 1 : feet]) i];
  foot_spacing = size[0]/(2*feet);
  foot_width = size[2]/sqrt(3); // width of bottom foot size
  difference() {
    union() {
      translate([-size[0]/2, -size[1]/2, 0])
        cube(size, center=false);

      // add feet
      for(i = foot_list)
        translate([-size[0]/2 + (2*i - 1) * foot_spacing, - size[1]/2, 0])
          rotate([0, 0, 0])
            union() {
              translate([-foot_width/2, 0, 0])
              cylinder(h=size[2] - tolerance, d=foot_d, $fn=6, center = false);
              translate([+foot_width/2, 0, 0])
              cylinder(h=size[2] - tolerance, d=foot_d, $fn=6, center = false);
            }
    }
    if (stackable) {
      // add feet cutout
      z_plus = 0.1; // z plus for cutouts
      for(i = foot_list)
        translate([-size[0]/2 + (2*i - 1) * foot_spacing, + size[1]/2, -z_plus])
          rotate([0, 0, 0])
            union() {
              translate([-foot_width/2, 0, 0])
              cylinder(h=size[2]+2*z_plus, d=foot_d+tolerance, $fn=6, center = false);
              translate([+foot_width/2, 0, 0])
              cylinder(h=size[2]+2*z_plus, d=foot_d+tolerance, $fn=6, center = false);
            }
    }
  }
}

translate([0, 40, 0])
  color("green")
    xy_center_cube_with_feet([40, 20, 4]);


translate([0, -40, 0])
  color("red")
    xy_center_cube_with_feet([80, 20, 4], feet = 5, foot_height = 6, stackable = false);


// helper module for rounding a cube
// @param corner_radius what radius to use for the edges/corners
// @param round_x/y/z whether to round the edges along the axis - single value true/false for all edges along that axis, or a list of 4 true/false e.g. [true, false, false, true] to control each edge along that axis
// @param center_x/y/z whether to center cube in x/y/z - true/false
module rounded_center_cube (size, corner_radius, round_x = false, round_y = false, round_z = false, center_x = true, center_y = true, center_z = true) {
  // parameters
  corner_d = 2 * corner_radius;

  // edges
  round_x = is_list(round_x) && len(round_x) == 4 ? round_x : [round_x, round_x, round_x, round_x];
  round_y = is_list(round_y) && len(round_y) == 4 ? round_y : [round_y, round_y, round_y, round_y];
  round_z = is_list(round_z) && len(round_z) == 4 ? round_z : [round_z, round_z, round_z, round_z];
  edges = [round_x[0], round_x[2], round_x[1], round_x[3], round_y[0], round_y[2], round_y[1], round_y[3], round_z[0], round_z[1], round_z[2], round_z[3]];
  x = [0, 0, 0, 0, 1, 1, -1, -1, 1, 1, -1, -1];
  y = [1, 1, -1, -1, 0, 0, 0, 0, 1, -1, 1, -1];
  z = [1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0];
  h = [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2];
  corners = [[0, 4, 8], [1, 5, 8], [2, 4, 9], [3, 5, 9], [0, 6, 10], [1, 7, 10], [2, 6, 11], [3, 7, 11]];

  // helper function for the cube rounding
  module rounded_edges(size, offset, corner_radius, edges, i) {
    if (edges[i])
      translate([x[i] * (size.x/2 - corner_radius), y[i] * (size.y/2 - corner_radius), z[i] * (size.z/2 - corner_radius)])
        rotate([90 * abs(x[i] * z[i]), 90 * abs(y[i] * z[i]), 0])
          cylinder(h = size[h[i]] + offset, d = corner_d, center = true);
    else
      translate([x[i] * (size.x/2 - corner_radius), y[i] * (size.y/2 - corner_radius), z[i] * (size.z/2 - corner_radius)])
        rotate([90 * abs(x[i] * z[i]), 90 * abs(y[i] * z[i]), 0])
          cube([ corner_d, corner_d, size[h[i]] + offset], center = true);
  }

  translate([!center_x ? size.x/2 : 0, !center_y ? size.y/2 : 0, !center_z ? size.z/2 : 0])
  union() {
    // create central structure
    cube([size.x, size.y - corner_d, size.z - corner_d], center = true);
    cube([size.x - corner_d, size.y, size.z - corner_d], center = true);
    cube([size.x - corner_d, size.y - corner_d, size.z], center = true);

    // create edges
    for (i = [0:(len(x) - 1)])
      rounded_edges(size, -corner_d, corner_radius, edges, i);

    // create round corners
    for (x = [-1, 1]) for(y = [-1, 1]) for(z = [-1, 1])
      translate([x * (size.x/2 - corner_radius), y * (size.y/2 - corner_radius), z * (size.z/2 - corner_radius)])
        sphere(r = corner_radius);

    // create edged corners
    for (corner = corners)
      if (!edges[corner[0]] || !edges[corner[1]] || !edges[corner[2]])
        intersection() {
          // not a round corner
          rounded_edges(size, 0, corner_radius, edges, corner[0]);
          rounded_edges(size, 0, corner_radius, edges, corner[1]);
          rounded_edges(size, 0, corner_radius, edges, corner[2]);
        }

  }
}

// generate attachment block
// @param block [width, height, depth]
// @param walls thickness
// @param screw_depth how deep the screws should go into the block (i.e. the wall before the nut + nut + beyond)
// @param whether to include a bottom rail (by default yes)
// @param center width of the center cutout (none by default), only matters if there is a bottom rail
// @param rails_tolerance (extra gap for good fit)
// @paran screws_tolerance extra tolerance for screws
module attachment_block(block, walls, screw_depth, side_rails = true, bottom_rail = true, center = 0, rails_tolerance = 0.4, screws_tolerance = 0) {
  screw_location = block[0]/2 - get_hexnut("M3")[1]/2 - 1.5 * walls;
  echo(str("INFO: attachment block ", block[0], "mm x ", block[1], "mm x ", block[2], "mm with screw locations +/- ", screw_location, "mm"));
  union() {
    difference() {
      // whole block
      xy_center_cube(block);
      // center cutout
      if (bottom_rail == true && center > 0) {
        translate([0, 0, -$e])
        xy_center_cube([center + 2 * rails_tolerance, block[1] + $2e, 2 * walls + $e]);
      }
      if (side_rails) {
        // front cutout
        front = (bottom_rail) ?
          [block[0] - 3 * walls + 2 * rails_tolerance, block[1] - 1.5 * walls + rails_tolerance + $e, 2 * walls + $e] :
          [block[0] - 3 * walls + 2 * rails_tolerance, block[1] + $2e, 2 * walls + $e];
        translate([0, (front[1] - block[1] - $e)/2, - $e])
        xy_center_cube(front);
        // rails cutout
        rails = (bottom_rail) ?
          [block[0] - 2 * walls + 2 * rails_tolerance, block[1] - walls + rails_tolerance + $e, walls + rails_tolerance]:
          [block[0] - 2 * walls + 2 * rails_tolerance, block[1] + $2e, walls + rails_tolerance];
        translate([0, (rails[1] - block[1] - $e)/2, walls - rails_tolerance])
        xy_center_cube(rails);
      }
      // hex nuts for attachment
      for(x = [-1, 1]) {
        translate([x * screw_location, 0, -$e])
        rotate([0, 0, -90]) // translate along x axis
        union() {
          translate([0, 0, 3 * walls + $e])
          hexnut("M3", screw_hole = false, z_plus = 0.2, tolerance = 0.15, stretch = 0.15, slot = block[1]/2 + $e);
          machine_screw("M3", length = screw_depth + 2 * walls + $2e, tolerance = 0.15 + screws_tolerance, stretch = 0.15, countersink = false);
        }
      }
    }
    // small cut-away bottom rail for easier print if bottom_rail = false
    if (side_rails && !bottom_rail) {
      rail_thickness = 0.25;
      translate([0, block[1]/2 - rail_thickness/2, 0])
      xy_center_cube([block[0], rail_thickness, rail_thickness]);
    }
  }
}


// generate attachment
// @param block [width, height, depth]
// @param walls thickness
// @param whether to include a bottom rail (by default yes)
// @param center width of the center cutout (none by default), only matters if there is a bottom rail
// @paran screws_tolerance extra tolerance for screws
module attachment(block, walls, bottom_rail = true, center = 0, screws_tolerance = 0) {
  front =
    (bottom_rail) ?
    [block[0] - 3 * walls, block[1] - 1.5 * walls, block[2]] :
    [block[0] - 3 * walls, block[1], block[2]];
  rails = (bottom_rail) ?
    [block[0] - 2 * walls, block[1] - walls, walls] :
    [block[0] - 2 * walls, block[1], walls];
  screw_location = block[0]/2 - get_hexnut("M3")[1]/2 - 1.5 * walls;
  echo(str("INFO: attachment top ", front[0], "mm x ", front[1], "mm x ", front[2], "mm with screw locations +/- ", screw_location, "mm"));
  difference() {
    union() {
      // center block
      if (bottom_rail == true && center > 0) {
        xy_center_cube([center, block[1], block[2]]);
      }
      // front
      translate([0, (front[1] - block[1])/2, 0])
      xy_center_cube(front);
      // rails
      translate([0, (rails[1] - block[1])/2, 0])
      xy_center_cube(rails);
    }
    // attachment screws
    for(x = [-1, 1]) {
      translate([x * screw_location, 0, -$e])
      machine_screw("M3", length = block[2] + $2e, tolerance = 0.15 + screws_tolerance, stretch = 0, countersink = false);
    }
  }
}


// example of attachment block and attachment
translate([0, -80, 0])
  rotate([0, 180, 0]) {
  color("yellow")
  attachment_block(block = [30, 14, 15], walls = 3, center = 10, screw_depth = 6);

  color("teal")
  translate([0, 0, 6])
  rotate([0, 180, 0])
  attachment(block = [30, 14, 10], walls = 3, center = 10);
}


// example of attachment block without rails
translate([0, -120, 0])
  rotate([0, 180, 0])
    color("magenta")
      attachment_block(block = [40, 20, 30], walls = 4, screw_depth = 20, side_rails = false, bottom_rail = false);
