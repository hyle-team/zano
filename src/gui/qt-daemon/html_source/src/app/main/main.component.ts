import {Component, NgZone, OnInit} from '@angular/core';
import {Location} from '@angular/common';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {ActivatedRoute, Router} from '@angular/router';
import {TranslateService} from '@ngx-translate/core';
import {CREATE_NEW_WALLET_HELP_PAGE} from '../_shared/constants';

@Component({
  selector: 'app-main',
  templateUrl: './main.component.html',
  styleUrls: ['./main.component.scss']
})
export class MainComponent implements OnInit {

  public prevUrl: string = '';

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private location: Location,
    private backend: BackendService,
    public variablesService: VariablesService,
    private ngZone: NgZone,
    private translate: TranslateService
  ) {
  }

  ngOnInit() {
    if (this.route.snapshot.queryParams && this.route.snapshot.queryParams.prevUrl) {
      this.prevUrl = this.route.snapshot.queryParams.prevUrl;
    }
  }

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
    this.backend.openUrlInBrowser(CREATE_NEW_WALLET_HELP_PAGE);
  }

  back() {
    this.location.back();
  }

}
