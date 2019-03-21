import {Component, NgZone, OnInit} from '@angular/core';
import {Location} from "@angular/common";
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {Router} from '@angular/router';
import {TranslateService} from '@ngx-translate/core';

@Component({
  selector: 'app-main',
  templateUrl: './main.component.html',
  styleUrls: ['./main.component.scss']
})
export class MainComponent implements OnInit {

  constructor(
    private router: Router,
    private location: Location,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone,
    private translate: TranslateService
  ) {}

  ngOnInit() {}

  openWallet() {
    this.backend.openFileDialog(this.translate.instant('MAIN.CHOOSE_PATH'), '*', this.variablesService.settings.default_path, (file_status, file_data) => {
      if (file_status) {
        this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
        this.ngZone.run(() => {
          this.router.navigate(['/open'], {queryParams: {path: file_data.path}});
        });
      } else {
        console.log(file_data['error_code']);
      }
    });
  }

  openInBrowser() {
    this.backend.openUrlInBrowser('docs.zano.org/v1.0/docs/how-to-create-wallet');
  }

  back() {
    this.location.back()
  }

}
