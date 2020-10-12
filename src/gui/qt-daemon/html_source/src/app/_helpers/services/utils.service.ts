import {Injectable} from '@angular/core';

@Injectable()
export class UtilsService {
  getMinWidthByScale(scale: number) {
    switch (scale) {
      case 7.5  : return 900;
      case 10   : return 1200;
      case 12.5 : return 1500;
      case 15   : return 1800;
      default   : return 1200;
    }
  }
}
